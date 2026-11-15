#pragma once
#include <algorithm>
#include <cmath>

namespace smoothride {
// Audio-thread owned metrics; callers publish snapshots to atomics for the UI.
// Load represents callback execution time / audio block duration, NOT process CPU usage.
class PerformanceTelemetry {
public:
    void reset() noexcept { averageLoad = 0.0; maxLoad = 0.0; lastMicroseconds = 0.0; initialized = false; }
    void record(double elapsedSeconds, int sampleCount, double sampleRate) noexcept {
        if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0 || sampleCount <= 0 || sampleRate <= 0.0) return;
        const double deadline = static_cast<double>(sampleCount) / sampleRate;
        if (deadline <= 0.0) return;
        const double load = elapsedSeconds / deadline;
        lastMicroseconds = elapsedSeconds * 1.0e6;
        if (!initialized) { averageLoad = load; initialized = true; }
        else averageLoad += 0.08 * (load - averageLoad);
        maxLoad = std::max(maxLoad, load);
    }
    float averagePercent() const noexcept { return static_cast<float>(100.0 * averageLoad); }
    float maximumPercent() const noexcept { return static_cast<float>(100.0 * maxLoad); }
    float callbackMicroseconds() const noexcept { return static_cast<float>(lastMicroseconds); }
private:
    double averageLoad = 0.0, maxLoad = 0.0, lastMicroseconds = 0.0;
    bool initialized = false;
};
}
