#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace smoothride {
struct Settings {
    float targetDb = -12.0f;      // RMS target (not LUFS)
    float floorDb = -60.0f;       // below this, never boost; return gradually to unity
    float maxBoostDb = 6.0f;      // positive ride cap
    float maxCutDb = 3.0f;        // negative ride cap
    float upMs = 3000.0f;        // gain increase time constant
    float downMs = 650.0f;       // gain decrease time constant
    float mix = 0.65f;           // dry/wet blend
    float outputDb = 6.0f;       // constant makeup gain
    float ceilingDb = -1.0f;     // instantaneous sample-peak ceiling, NOT true-peak
    float gateHysteresisDb = 0.0f; // 0 preserves v0.3.1 threshold behavior
    float lowLevelProtect = 0.0f; // 0 preserves v0.3.1; 1 applies 12 dB soft floor
    float adaptiveFloor = 0.0f; // OFF by default; optional low-level program detection
    float noiseGuard = 0.0f; // experimental: blocks steady low-level signal, OFF by default
};
class GainRider {
public:
    void prepare(double sr) noexcept { sampleRate = std::max(8000.0, sr); reset(); }
    void reset() noexcept { power = 0.0; rideDb = 0.0; peakGain = 1.0; detectedRmsDb = -160.0; requestedRideDb = 0.0; ridingActive = false; effectiveFloorDb = settings.floorDb; noiseReferenceDb = -120.0; silenceBlocked = false; noiseBlocked = false; }
    void setSettings(Settings s) noexcept { settings = s; }
    float currentRideDb() const noexcept { return (float)rideDb; }
    float currentPeakReductionDb() const noexcept {
        return (float)(20.0 * std::log10(std::max(1.0e-9, peakGain)));
    }
    // Diagnostics only: never fed back into signal processing.
    float currentDetectedRmsDb() const noexcept { return (float)detectedRmsDb; }
    float currentRequestedRideDb() const noexcept { return (float)requestedRideDb; }
    float currentEffectiveFloorDb() const noexcept { return (float)effectiveFloorDb; }
    bool isRidingActive() const noexcept { return ridingActive; }
    bool isSilenceBlocked() const noexcept { return silenceBlocked; }
    bool isNoiseBlocked() const noexcept { return noiseBlocked; }
    float currentNoiseReferenceDb() const noexcept { return (float)noiseReferenceDb; }
    void process(float* left, float* right, int numSamples, bool stereo) noexcept {
        if (!left || numSamples <= 0) return;
        const double detect = coeff(250.0); // stereo-linked 250 ms RMS
        const double up = coeff(std::clamp((double)settings.upMs, 100.0, 30000.0));
        const double down = coeff(std::clamp((double)settings.downMs, 50.0, 10000.0));
        const double idle = coeff(6000.0);
        const double peakRelease = coeff(100.0);
        const double target = settings.targetDb;
        const double manualFloor = settings.floorDb;
        const bool adaptive = settings.adaptiveFloor >= 0.5f;
        const bool guardEnabled = adaptive && settings.noiseGuard >= 0.5f;
        // Conservative, experimental steady-low-level detector, not a speech/noise classifier.
        const double referenceRise = coeff(8000.0);
        const double referenceFall = coeff(500.0);
        const double hysteresis = std::clamp((double)settings.gateHysteresisDb, 0.0, 12.0);
        const bool protect = settings.lowLevelProtect >= 0.5f;
        const double maxBoost = std::max(0.0, (double)settings.maxBoostDb);
        const double maxCut = std::max(0.0, (double)settings.maxCutDb);
        const double mix = std::clamp((double)settings.mix, 0.0, 1.0);
        const double makeup = dbToLinear(settings.outputDb);
        const double ceiling = dbToLinear(std::min(-0.1f, settings.ceilingDb));
        for (int i = 0; i < numSamples; ++i) {
            const double l = std::isfinite(left[i]) ? left[i] : 0.0;
            const double r = stereo && right && std::isfinite(right[i]) ? right[i] : l;
            const double samplePower = stereo ? (l*l+r*r)*0.5 : l*l;
            power = detect*power + (1.0-detect)*samplePower;
            const double detectedDb = 10.0*std::log10(std::max(1.0e-16, power));
            // Adaptive mode is level-based, not an actual speech/noise classifier.
            // It can lower the floor for quiet program material, but retains a
            // hard -88 dBFS silence cutoff. Pure steady noise above that may
            // also activate it: avoid promising content-aware noise rejection.
            const double floor = adaptive
                ? std::min(manualFloor, std::clamp(detectedDb - 8.0, -85.0, -30.0))
                : manualFloor;
            effectiveFloorDb = floor;
            const bool aboveSilenceCutoff = !adaptive || detectedDb > -88.0;
            // A slower rising reference follows persistent low-level energy; a faster
            // falling reference follows quiet gaps. Only enabled guard affects riding.
            // Constant quiet music may be classified like noise: use only if useful.
            if (guardEnabled && detectedDb > -88.0) {
                const double alpha = detectedDb > noiseReferenceDb ? referenceRise : referenceFall;
                noiseReferenceDb = alpha*noiseReferenceDb + (1.0-alpha)*detectedDb;
            }
            silenceBlocked = adaptive && !aboveSilenceCutoff;
            noiseBlocked = guardEnabled && aboveSilenceCutoff && detectedDb < -55.0
                           && (detectedDb - noiseReferenceDb) < 1.5;
            // Threshold gate keeps silence/noise from being amplified.
            // With hysteresis disabled, this reproduces the original v0.3.1 comparison.
            // Enabled: activate above floor + hyst/2; remain active down to floor - hyst/2.
            const bool active = aboveSilenceCutoff && !noiseBlocked && (hysteresis <= 0.0
                ? detectedDb > floor
                : (ridingActive ? detectedDb > floor - hysteresis*0.5
                                : detectedDb > floor + hysteresis*0.5));
            double desired = active ? std::clamp(target - detectedDb, -maxCut, maxBoost) : 0.0;
            // Optional soft floor: ramp positive gain from zero to full over 12 dB.
            // Below floor there is still no automatic positive boost.
            if (protect && desired > 0.0)
                desired *= std::clamp((detectedDb - floor)/12.0, 0.0, 1.0);
            detectedRmsDb = detectedDb;
            requestedRideDb = desired;
            ridingActive = active;
            const double smoothing = !active ? idle : (desired > rideDb ? up : down);
            rideDb = smoothing*rideDb + (1.0-smoothing)*desired;
            // Signal-weighted dry/wet blend with global makeup gain.
            const double gain = makeup*((1.0-mix) + mix*dbToLinear(rideDb));
            const double outL = l*gain;
            const double outR = r*gain;
            const double peak = stereo ? std::max(std::abs(outL), std::abs(outR)) : std::abs(outL);
            const double request = peak > ceiling ? ceiling/peak : 1.0;
            if (request < peakGain) peakGain = request;
            else peakGain = peakRelease*peakGain + (1.0-peakRelease)*request;
            left[i] = (float)(outL*peakGain);
            if (stereo && right) right[i] = (float)(outR*peakGain);
        }
    }
private:
    double coeff(double ms) const noexcept { return std::exp(-1.0/(sampleRate*ms*0.001)); }
    static double dbToLinear(double db) noexcept { return std::pow(10.0, db/20.0); }
    Settings settings;
    double sampleRate = 48000.0, power = 0.0, rideDb = 0.0, peakGain = 1.0;
    double detectedRmsDb = -160.0, requestedRideDb = 0.0, effectiveFloorDb = -60.0;
    double noiseReferenceDb = -120.0;
    bool ridingActive = false, silenceBlocked = false, noiseBlocked = false;
};
}
