#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace smoothride {
// Audio-thread owned metrics. The GUI reads only atomic snapshots published by the processor.
// All percentages represent elapsed callback wall time / audio-block duration (NOT system CPU).
class PerformanceTelemetry {
public:
    void reset() noexcept {
        averageLoad = maxLoad = lastMicroseconds = rollingMaxLoad = 0.0;
        spikeCount = 0; initialized = false; lastRollingSecond = -1;
        for (auto& b : seconds) { b.second = -1; b.peak = 0.0; }
    }

    // The clock input allows deterministic tests without sleeps.
    void record(double elapsedSeconds, int sampleCount, double sampleRate,
                std::int64_t monotonicSecond, double spikeThresholdPercent = 10.0) noexcept {
        if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0
                || sampleCount <= 0 || !std::isfinite(sampleRate) || sampleRate <= 0.0) return;
        const double deadline = static_cast<double>(sampleCount) / sampleRate;
        if (deadline <= 0.0) return;
        const double load = elapsedSeconds / deadline;
        lastMicroseconds = elapsedSeconds * 1.0e6;
        if (!initialized) { averageLoad = load; initialized = true; }
        else averageLoad += 0.08 * (load - averageLoad);
        maxLoad = std::max(maxLoad, load);
        if (std::isfinite(spikeThresholdPercent) && spikeThresholdPercent > 0.0
                && load * 100.0 >= spikeThresholdPercent) ++spikeCount;

        // Fixed 60-bucket ring indexed by monotonic wall-clock seconds. No allocation or locks.
        // Only rescan the 60 buckets when the second changes, not on every callback.
        const auto slot = static_cast<std::size_t>((monotonicSecond % 60 + 60) % 60);
        auto& b = seconds[slot];
        if (b.second != monotonicSecond) { b.second = monotonicSecond; b.peak = 0.0; }
        b.peak = std::max(b.peak, load);
        if (monotonicSecond != lastRollingSecond) {
            lastRollingSecond = monotonicSecond;
            rollingMaxLoad = 0.0;
            for (const auto& entry : seconds)
                if (entry.second <= monotonicSecond && monotonicSecond - entry.second < 60)
                    rollingMaxLoad = std::max(rollingMaxLoad, entry.peak);
        } else {
            rollingMaxLoad = std::max(rollingMaxLoad, b.peak);
        }
    }
    // Legacy 3-argument interface for standalone tests / external callers.
    void record(double elapsedSeconds, int sampleCount, double sampleRate) noexcept {
        const auto now = std::chrono::steady_clock::now();
        const auto second = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        record(elapsedSeconds, sampleCount, sampleRate, second);
    }
    float averagePercent() const noexcept { return static_cast<float>(100.0 * averageLoad); }
    float maximumPercent() const noexcept { return static_cast<float>(100.0 * maxLoad); }
    float rollingPeakPercent() const noexcept { return static_cast<float>(100.0 * rollingMaxLoad); }
    float callbackMicroseconds() const noexcept { return static_cast<float>(lastMicroseconds); }
    std::uint64_t spikes() const noexcept { return spikeCount; }
private:
    struct Bucket { std::int64_t second = -1; double peak = 0.0; };
    std::array<Bucket, 60> seconds {};
    double averageLoad = 0.0, maxLoad = 0.0, rollingMaxLoad = 0.0, lastMicroseconds = 0.0;
    std::uint64_t spikeCount = 0;
    std::int64_t lastRollingSecond = -1;
    bool initialized = false;
};
} // namespace smoothride
