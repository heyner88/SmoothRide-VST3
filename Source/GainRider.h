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
};
class GainRider {
public:
    void prepare(double sr) noexcept { sampleRate = std::max(8000.0, sr); reset(); }
    void reset() noexcept { power = 0.0; rideDb = 0.0; peakGain = 1.0; }
    void setSettings(Settings s) noexcept { settings = s; }
    float currentRideDb() const noexcept { return (float)rideDb; }
    float currentPeakReductionDb() const noexcept {
        return (float)(20.0 * std::log10(std::max(1.0e-9, peakGain)));
    }
    void process(float* left, float* right, int numSamples, bool stereo) noexcept {
        if (!left || numSamples <= 0) return;
        const double detect = coeff(250.0); // stereo-linked 250 ms RMS
        const double up = coeff(std::clamp((double)settings.upMs, 100.0, 30000.0));
        const double down = coeff(std::clamp((double)settings.downMs, 50.0, 10000.0));
        const double idle = coeff(6000.0);
        const double peakRelease = coeff(100.0);
        const double target = settings.targetDb;
        const double floor = settings.floorDb;
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
            // Threshold gate keeps silence/noise from being amplified.
            const bool active = detectedDb > floor;
            const double desired = active ? std::clamp(target - detectedDb, -maxCut, maxBoost) : 0.0;
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
};
}
