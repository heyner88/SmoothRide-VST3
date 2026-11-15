#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace smoothride {
struct Settings {
    float targetDb = -15.0f;    // RMS target, not LUFS
    float floorDb = -60.0f;     // below this hold then relax toward unity
    float rangeDb = 6.0f;       // symmetrical maximum ride in dB
    float speedMs = 1800.0f;    // gain-riding time constant
    float mix = 0.65f;          // 0 dry, 1 wet
    float outputDb = 6.0f;     // makeup gain
    float ceilingDb = -1.0f;   // sample peak safety ceiling
};

class GainRider {
public:
    void prepare(double newSampleRate) noexcept {
        sampleRate = std::max(8000.0, newSampleRate);
        power = 0.0; rideDb = 0.0; peakGain = 1.0;
    }
    void reset() noexcept { power = 0.0; rideDb = 0.0; peakGain = 1.0; }
    void setSettings(Settings s) noexcept { settings = s; }
    float currentRideDb() const noexcept { return (float) rideDb; }
    float currentPeakReductionDb() const noexcept { return (float)(20.0 * std::log10(std::max(1.0e-9, peakGain))); }

    // Stereo-linked processing; channels may point to the same array for mono.
    void process(float* left, float* right, int numSamples, bool stereo) noexcept {
        if (numSamples <= 0 || !left) return;
        const auto det = coeff(250.0); // 250 ms RMS detection
        const auto ride = coeff(std::clamp((double)settings.speedMs, 50.0, 10000.0));
        const auto returnToZero = coeff(6000.0);
        const auto limiterRelease = coeff(100.0);
        const double target = settings.targetDb;
        const double floor = settings.floorDb;
        const double range = std::max(0.0, (double)settings.rangeDb);
        const double mix = std::clamp((double)settings.mix, 0.0, 1.0);
        const double makeup = dbToLinear(settings.outputDb);
        const double ceiling = dbToLinear(std::min(-0.1f, settings.ceilingDb));
        for (int i = 0; i < numSamples; ++i) {
            const double l = std::isfinite(left[i]) ? left[i] : 0.0;
            const double r = stereo && right && std::isfinite(right[i]) ? right[i] : l;
            const double instantaneousPower = stereo ? (l*l+r*r)*0.5 : l*l;
            power = det*power + (1.0-det)*instantaneousPower;
            const double detectedDb = 10.0 * std::log10(std::max(1.0e-16, power));
            // Don't ride up pauses or noise: gradually release to neutral instead.
            const double wantedDb = detectedDb > floor ? std::clamp(target-detectedDb, -range, range) : 0.0;
            const double smoothing = detectedDb > floor ? ride : returnToZero;
            rideDb = smoothing*rideDb + (1.0-smoothing)*wantedDb;
            const double wetGain = dbToLinear(rideDb);
            const double combinedGain = makeup * ((1.0-mix) + mix*wetGain);
            double outL = l * combinedGain;
            double outR = r * combinedGain;
            // Linked zero-lookahead SAMPLE-peak safety, NOT a true-peak limiter.
            const double peak = stereo ? std::max(std::abs(outL),std::abs(outR)) : std::abs(outL);
            const double requested = peak > ceiling ? ceiling / peak : 1.0;
            if (requested < peakGain) peakGain = requested; // immediate peak protection
            else peakGain = limiterRelease*peakGain + (1.0-limiterRelease)*requested;
            left[i] = (float)(outL * peakGain);
            if (stereo && right) right[i] = (float)(outR * peakGain);
        }
    }
private:
    double coeff(double ms) const noexcept { return std::exp(-1.0 / (sampleRate * ms * 0.001)); }
    static double dbToLinear(double db) noexcept { return std::pow(10.0, db / 20.0); }
    Settings settings;
    double sampleRate = 48000.0, power = 0.0, rideDb = 0.0, peakGain = 1.0;
};
} // namespace smoothride
