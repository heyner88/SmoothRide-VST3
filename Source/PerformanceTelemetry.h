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
        lastTogglePreLoad = lastTogglePostLoad = 0.0; lastMonitorUs = 0.0;
        lastToggleParamsLoad = lastToggleInputLoad = lastToggleABLoad = 0.0;
        baselinePreLoad = 0.0; baselineInitialized = false; lastToggleABRunning = false;
        normalParamsLoad = 0.0; normalParamsInitialized = false;
        toggleParamsSum = toggleParamsMaximum = 0.0; resetDurationUs = 0.0;
        for (auto& b : seconds) { b.second = -1; b.peak = 0.0; }
    }

    // The clock input allows deterministic tests without sleeps.
    void record(double elapsedSeconds, int sampleCount, double sampleRate,
                std::int64_t monotonicSecond, double spikeThresholdPercent = 10.0,
                bool guardToggled = false, double riderSeconds = 0.0,
                double preSeconds = 0.0, double postSeconds = 0.0,
                double monitorSeconds = 0.0,
                double parametersSeconds = 0.0, double inputAnalysisSeconds = 0.0,
                double abPreparationSeconds = 0.0, bool abEngineRan = false,
                double resetSeconds = 0.0) noexcept {
        if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0
                || sampleCount <= 0 || !std::isfinite(sampleRate) || sampleRate <= 0.0) return;
        const double deadline = static_cast<double>(sampleCount) / sampleRate;
        if (deadline <= 0.0) return;
        const double load = elapsedSeconds / deadline;
        // Reset is measured before callbackStart and excluded from BLOCK / P.
        if (std::isfinite(resetSeconds) && resetSeconds > 0.0)
            resetDurationUs = resetSeconds * 1.e6;
        lastMicroseconds = elapsedSeconds * 1.0e6;
        // Monitoring cost was timed after record/publish in the PREVIOUS callback,
        // because this callback cannot include its own future telemetry work.
        lastMonitorUs = std::isfinite(monitorSeconds) && monitorSeconds >= 0.0
            ? monitorSeconds * 1.0e6 : 0.0;
        if (!initialized) { averageLoad = load; initialized = true; }
        else averageLoad += 0.08 * (load - averageLoad);
        maxLoad = std::max(maxLoad, load);
        if (!guardToggled) {
            const double params = std::max(0.0, parametersSeconds / deadline * 100.0);
            if (!normalParamsInitialized) { normalParamsLoad = params; normalParamsInitialized = true; }
            else normalParamsLoad += 0.02 * (params - normalParamsLoad);
            const double pre = std::max(0.0, preSeconds / deadline * 100.0);
            if (!baselineInitialized) { baselinePreLoad = pre; baselineInitialized = true; }
            else baselinePreLoad += 0.02 * (pre - baselinePreLoad);
        }
        if (guardToggled) {
            ++toggleCount;
            lastToggleLoad = load * 100.0;
            lastToggleRiderLoad = std::isfinite(riderSeconds) && riderSeconds >= 0.0
                ? std::clamp(riderSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
            lastTogglePreLoad = std::isfinite(preSeconds) && preSeconds >= 0.0
                ? std::clamp(preSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
            lastToggleParamsLoad = std::isfinite(parametersSeconds) && parametersSeconds >= 0.0
                ? std::clamp(parametersSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
            lastToggleInputLoad = std::isfinite(inputAnalysisSeconds) && inputAnalysisSeconds >= 0.0
                ? std::clamp(inputAnalysisSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
            lastToggleABLoad = std::isfinite(abPreparationSeconds) && abPreparationSeconds >= 0.0
                ? std::clamp(abPreparationSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
            lastToggleABRunning = abEngineRan;
            toggleParamsSum += lastToggleParamsLoad;
            toggleParamsMaximum = std::max(toggleParamsMaximum, lastToggleParamsLoad);
            lastTogglePostLoad = std::isfinite(postSeconds) && postSeconds >= 0.0
                ? std::clamp(postSeconds / deadline * 100.0, 0.0, lastToggleLoad) : 0.0;
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
    float lastTogglePrePercent() const noexcept { return static_cast<float>(lastTogglePreLoad); }
    float lastTogglePostPercent() const noexcept { return static_cast<float>(lastTogglePostLoad); }
    float lastToggleParamsPercent() const noexcept { return static_cast<float>(lastToggleParamsLoad); }
    float lastToggleInputPercent() const noexcept { return static_cast<float>(lastToggleInputLoad); }
    float lastToggleABPercent() const noexcept { return static_cast<float>(lastToggleABLoad); }
    bool wasABEngineRunning() const noexcept { return lastToggleABRunning; }
    float normalPreAveragePercent() const noexcept { return static_cast<float>(baselinePreLoad); }
    float lastMonitorMicroseconds() const noexcept { return static_cast<float>(lastMonitorUs); }
    float averagePercent() const noexcept { return static_cast<float>(100.0 * averageLoad); }
    float maximumPercent() const noexcept { return static_cast<float>(100.0 * maxLoad); }
    float rollingPeakPercent() const noexcept { return static_cast<float>(100.0 * rollingMaxLoad); }
    float normalParamsPercent() const noexcept { return static_cast<float>(normalParamsLoad); }
    float toggleParamsMeanPercent() const noexcept { return static_cast<float>(toggleCount ? toggleParamsSum / static_cast<double>(toggleCount) : 0.0); }
    float toggleParamsMaxPercent() const noexcept { return static_cast<float>(toggleParamsMaximum); }
    float resetMicroseconds() const noexcept { return static_cast<float>(resetDurationUs); }
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
    double lastTogglePreLoad = 0.0, lastTogglePostLoad = 0.0, lastMonitorUs = 0.0;
    double lastToggleParamsLoad = 0.0, lastToggleInputLoad = 0.0, lastToggleABLoad = 0.0;
    double normalParamsLoad = 0.0, toggleParamsSum = 0.0, toggleParamsMaximum = 0.0, resetDurationUs = 0.0;
    bool normalParamsInitialized = false;
    double baselinePreLoad = 0.0;
    bool baselineInitialized = false, lastToggleABRunning = false;
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
    float lastTogglePrePercent = 0, lastTogglePostPercent = 0, monitorUs = 0;
    float lastToggleParamsPercent = 0, lastToggleInputPercent = 0, lastToggleABPercent = 0;
    float normalPreAveragePercent = 0;
    float normalParamsPercent = 0, toggleParamsMeanPercent = 0, toggleParamsMaxPercent = 0, resetMicroseconds = 0;
    bool lastToggleABRunning = false;
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
        togglePreLoad.store(t.lastTogglePrePercent(), std::memory_order_seq_cst);
        togglePostLoad.store(t.lastTogglePostPercent(), std::memory_order_seq_cst);
        toggleParams.store(t.lastToggleParamsPercent(), std::memory_order_seq_cst);
        toggleInput.store(t.lastToggleInputPercent(), std::memory_order_seq_cst);
        toggleAB.store(t.lastToggleABPercent(), std::memory_order_seq_cst);
        baselinePre.store(t.normalPreAveragePercent(), std::memory_order_seq_cst);
        normalParams.store(t.normalParamsPercent(), std::memory_order_seq_cst);
        meanParams.store(t.toggleParamsMeanPercent(), std::memory_order_seq_cst);
        maxParams.store(t.toggleParamsMaxPercent(), std::memory_order_seq_cst);
        resetUs.store(t.resetMicroseconds(), std::memory_order_seq_cst);
        abRunning.store(t.wasABEngineRunning(), std::memory_order_seq_cst);
        monitorUs.store(t.lastMonitorMicroseconds(), std::memory_order_seq_cst);
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
            result.lastTogglePrePercent = togglePreLoad.load(std::memory_order_seq_cst);
            result.lastTogglePostPercent = togglePostLoad.load(std::memory_order_seq_cst);
            result.monitorUs = monitorUs.load(std::memory_order_seq_cst);
            result.lastToggleParamsPercent = toggleParams.load(std::memory_order_seq_cst);
            result.lastToggleInputPercent = toggleInput.load(std::memory_order_seq_cst);
            result.lastToggleABPercent = toggleAB.load(std::memory_order_seq_cst);
            result.normalPreAveragePercent = baselinePre.load(std::memory_order_seq_cst);
            result.normalParamsPercent = normalParams.load(std::memory_order_seq_cst);
            result.toggleParamsMeanPercent = meanParams.load(std::memory_order_seq_cst);
            result.toggleParamsMaxPercent = maxParams.load(std::memory_order_seq_cst);
            result.resetMicroseconds = resetUs.load(std::memory_order_seq_cst);
            result.lastToggleABRunning = abRunning.load(std::memory_order_seq_cst);
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
    std::atomic<float> togglePreLoad {0}, togglePostLoad {0}, monitorUs {0};
    std::atomic<float> toggleParams {0}, toggleInput {0}, toggleAB {0}, baselinePre {0};
    std::atomic<float> normalParams {0}, meanParams {0}, maxParams {0}, resetUs {0};
    std::atomic<bool> abRunning {false};
    mutable DSPSnapshot lastGood {};
};
} // namespace smoothride
