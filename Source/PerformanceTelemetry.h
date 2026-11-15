#pragma once
#include <algorithm>
#include <array>
#include <atomic>
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
        spikeCount = 0; lastSpikePercent = lastSpikeMicroseconds = lastSpikeBudgetMicroseconds = 0.0;
        initialized = false; lastRollingSecond = -1;
        toggleCount = 0; lastToggleLoad = lastToggleRiderLoad = 0.0;
        for (auto& b : seconds) { b.second = -1; b.peak = 0.0; }
    }

    // The clock input allows deterministic tests without sleeps.
    void record(double elapsedSeconds, int sampleCount, double sampleRate,
                std::int64_t monotonicSecond, double spikeThresholdPercent = 10.0,
                bool guardToggled = false, double riderSeconds = 0.0) noexcept {
        if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0
                || sampleCount <= 0 || !std::isfinite(sampleRate) || sampleRate <= 0.0) return;
        const double deadline = static_cast<double>(sampleCount) / sampleRate;
        if (deadline <= 0.0) return;
        const double load = elapsedSeconds / deadline;
        lastMicroseconds = elapsedSeconds * 1.0e6;
        if (!initialized) { averageLoad = load; initialized = true; }
        else averageLoad += 0.08 * (load - averageLoad);
        maxLoad = std::max(maxLoad, load);
        if (guardToggled) {
            ++toggleCount;
            lastToggleLoad = load * 100.0;
            lastToggleRiderLoad = std::isfinite(riderSeconds) && riderSeconds >= 0.0
                ? std::clamp(riderSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
        }
        if (std::isfinite(spikeThresholdPercent) && spikeThresholdPercent > 0.0
                && load * 100.0 >= spikeThresholdPercent) {
            ++spikeCount;
            lastSpikePercent = load * 100.0;
            lastSpikeMicroseconds = elapsedSeconds * 1.0e6;
            lastSpikeBudgetMicroseconds = deadline * 1.0e6;
        }

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
    std::uint64_t guardToggles() const noexcept { return toggleCount; }
    float lastTogglePercent() const noexcept { return static_cast<float>(lastToggleLoad); }
    float lastToggleRiderPercent() const noexcept { return static_cast<float>(lastToggleRiderLoad); }
    float averagePercent() const noexcept { return static_cast<float>(100.0 * averageLoad); }
    float maximumPercent() const noexcept { return static_cast<float>(100.0 * maxLoad); }
    float rollingPeakPercent() const noexcept { return static_cast<float>(100.0 * rollingMaxLoad); }
    float callbackMicroseconds() const noexcept { return static_cast<float>(lastMicroseconds); }
    std::uint64_t spikes() const noexcept { return spikeCount; }
    float lastSpikeLoadPercent() const noexcept { return static_cast<float>(lastSpikePercent); }
    float lastSpikeDurationUs() const noexcept { return static_cast<float>(lastSpikeMicroseconds); }
    float lastSpikeBudgetUs() const noexcept { return static_cast<float>(lastSpikeBudgetMicroseconds); }
private:
    struct Bucket { std::int64_t second = -1; double peak = 0.0; };
    std::array<Bucket, 60> seconds {};
    double averageLoad = 0.0, maxLoad = 0.0, rollingMaxLoad = 0.0, lastMicroseconds = 0.0;
    std::uint64_t spikeCount = 0, toggleCount = 0;
    double lastToggleLoad = 0.0, lastToggleRiderLoad = 0.0;
    double lastSpikePercent = 0.0, lastSpikeMicroseconds = 0.0, lastSpikeBudgetMicroseconds = 0.0;
    std::int64_t lastRollingSecond = -1;
    bool initialized = false;
};
} // namespace smoothride

namespace smoothride {
// Published statistics contain atomic fields, so concurrent GUI reads have no data races.
// The sequence number brackets the writer's updates; the GUI retries if it overlaps one.
struct DSPSnapshot {
    float average = 0, peak = 0, rollingPeak = 0, callbackUs = 0;
    float lastSpikePercent = 0, lastSpikeUs = 0, lastSpikeBudgetUs = 0;
    std::uint64_t spikes = 0, guardToggles = 0;
    float lastTogglePercent = 0, lastToggleRiderPercent = 0;
};
class PublishedDSPTelemetry {
public:
    void publish(const PerformanceTelemetry& t) noexcept {
        version.fetch_add(1, std::memory_order_seq_cst); // odd: update in progress
        avg.store(t.averagePercent(), std::memory_order_seq_cst);
        peak.store(t.maximumPercent(), std::memory_order_seq_cst);
        rolling.store(t.rollingPeakPercent(), std::memory_order_seq_cst);
        callback.store(t.callbackMicroseconds(), std::memory_order_seq_cst);
        spikeCount.store(t.spikes(), std::memory_order_seq_cst);
        spikeLoad.store(t.lastSpikeLoadPercent(), std::memory_order_seq_cst);
        spikeDuration.store(t.lastSpikeDurationUs(), std::memory_order_seq_cst);
        spikeBudget.store(t.lastSpikeBudgetUs(), std::memory_order_seq_cst);
        toggleCount.store(t.guardToggles(), std::memory_order_seq_cst);
        toggleLoad.store(t.lastTogglePercent(), std::memory_order_seq_cst);
        toggleRiderLoad.store(t.lastToggleRiderPercent(), std::memory_order_seq_cst);
        version.fetch_add(1, std::memory_order_seq_cst); // even: complete
    }
    DSPSnapshot read() const noexcept {
        DSPSnapshot result;
        // Bounded retries keep the GUI responsive even under heavy audio traffic.
        for (int i = 0; i < 32; ++i) {
            const auto before = version.load(std::memory_order_seq_cst);
            if (before & 1u) continue;
            result.average = avg.load(std::memory_order_seq_cst);
            result.peak = peak.load(std::memory_order_seq_cst);
            result.rollingPeak = rolling.load(std::memory_order_seq_cst);
            result.callbackUs = callback.load(std::memory_order_seq_cst);
            result.spikes = spikeCount.load(std::memory_order_seq_cst);
            result.lastSpikePercent = spikeLoad.load(std::memory_order_seq_cst);
            result.lastSpikeUs = spikeDuration.load(std::memory_order_seq_cst);
            result.lastSpikeBudgetUs = spikeBudget.load(std::memory_order_seq_cst);
            result.guardToggles = toggleCount.load(std::memory_order_seq_cst);
            result.lastTogglePercent = toggleLoad.load(std::memory_order_seq_cst);
            result.lastToggleRiderPercent = toggleRiderLoad.load(std::memory_order_seq_cst);
            if (version.load(std::memory_order_seq_cst) == before) return result;
        }
        // During a rare busy interval, avoid showing an internally inconsistent frame.
        return lastGood;
    }
    // Called by the message thread after a successful read, not the audio thread.
    void remember(const DSPSnapshot& s) const noexcept { lastGood = s; }
private:
    std::atomic<unsigned int> version {0};
    std::atomic<float> avg {0}, peak {0}, rolling {0}, callback {0};
    std::atomic<float> spikeLoad {0}, spikeDuration {0}, spikeBudget {0};
    std::atomic<std::uint64_t> spikeCount {0}, toggleCount {0};
    std::atomic<float> toggleLoad {0}, toggleRiderLoad {0};
    mutable DSPSnapshot lastGood {};
};
} // namespace smoothride
