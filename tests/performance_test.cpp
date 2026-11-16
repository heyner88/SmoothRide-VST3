#include "../Source/PerformanceTelemetry.h"
#include <cassert>
#include <cmath>
#include <iostream>
static void testSpikeEventSnapshots() {
    smoothride::PerformanceTelemetry t;
    smoothride::PublishedDSPTelemetry published;
    // 10ms budget: event A is peak and a toggle spike.
    t.record(.003, 480, 48000, 100, 10., true,
             .0006, .0018, .0006, 0., .0012, .0003, .0003, false);
    published.publish(t);
    auto first = published.read();
    assert(first.spikes == 1 && first.toggleSpikes == 1 && first.normalSpikes == 0);
    assert(std::abs(first.historicalPeakEvent.block - 30.f) < .001f);
    assert(std::abs(first.historicalPeakEvent.params - 12.f) < .001f);
    assert(first.historicalPeakEvent.guardToggle);
    assert(first.historicalPeakEvent.callbackId == 1);
    assert(first.lastSpikeEvent.callbackId == 1);
    assert(std::abs(first.peak - first.historicalPeakEvent.block) < 0.0001f);
    assert(std::abs(first.historicalPeakEvent.params + first.historicalPeakEvent.input
                    + first.historicalPeakEvent.ab + first.historicalPeakEvent.rider
                    + first.historicalPeakEvent.output - first.historicalPeakEvent.block) < .001f);
    // Event B is a smaller normal callback above threshold, updates last spike only.
    t.record(.002, 480, 48000, 101, 10., false,
             .0005, .001, .0005, 0., .0004, .0004, .0002, true);
    published.publish(t);
    auto second = published.read();
    assert(second.spikes == 2 && second.toggleSpikes == 1 && second.normalSpikes == 1);
    assert(std::abs(second.historicalPeakEvent.block - 30.f) < .001f);
    assert(std::abs(second.lastSpikeEvent.block - 20.f) < .001f);
    assert(!second.lastSpikeEvent.guardToggle && second.lastSpikeEvent.abEngineRunning);
    assert(second.historicalPeakEvent.callbackId == 1);
    assert(second.lastSpikeEvent.callbackId == 2);
    assert(std::abs(second.peak - second.historicalPeakEvent.block) < 0.0001f);
    // Smaller non-spike does not overwrite event snapshots.
    t.record(.0002, 480, 48000, 102, 10., false);
    published.publish(t);
    assert(published.read().lastSpikeEvent.block == second.lastSpikeEvent.block);
    assert(published.read().historicalPeakEvent.callbackId == 1);
    t.reset(); published.publish(t);
    const auto cleared = published.read();
    assert(cleared.spikes == 0 && cleared.toggleSpikes == 0 && cleared.normalSpikes == 0);
    assert(cleared.historicalPeakEvent.block == 0 && cleared.lastSpikeEvent.block == 0);
    assert(cleared.historicalPeakEvent.callbackId == 0 && cleared.lastSpikeEvent.callbackId == 0);
}
int main() {
    testSpikeEventSnapshots();
    smoothride::PerformanceTelemetry t;
    t.record(0.0001, 480, 48000); // 100 us out of 10 ms = 1%
    assert(std::abs(t.averagePercent() - 1.f) < 0.0001f);
    assert(std::abs(t.maximumPercent() - 1.f) < 0.0001f);
    assert(std::abs(t.callbackMicroseconds() - 100.f) < 0.0001f);
    t.record(0.0002, 480, 48000); // 2% maximum, averaged with 8% step
    assert(std::abs(t.maximumPercent() - 2.f) < 0.0001f);
    assert(t.averagePercent() > 1.f && t.averagePercent() < 2.f);
    t.record(-1, 480, 48000); // invalid sample ignored
    assert(std::abs(t.maximumPercent() - 2.f) < 0.0001f);
    // Deterministic 60-second rolling window and configurable threshold.
    t.reset();
    t.record(0.0011, 480, 48000, 1000, 10.0); // 11% block load
    assert(t.spikes() == 1);
    assert(std::abs(t.rollingPeakPercent() - 11.f) < 0.0001f);
    t.record(0.0005, 480, 48000, 1001, 10.0); // 5%
    assert(t.spikes() == 1);
    assert(std::abs(t.rollingPeakPercent() - 11.f) < 0.0001f);
    t.record(0.0004, 480, 48000, 1060, 10.0); // 4%, old 11% expired
    assert(std::abs(t.rollingPeakPercent() - 5.f) < 0.0001f);
    t.record(0.0005, 480, 48000, 1061, 5.0); // threshold changed to 5%
    assert(t.spikes() == 2);
    t.record(0.0003, 480, 48000, 1200, 5.0); // all history expired
    assert(std::abs(t.rollingPeakPercent() - 3.f) < 0.0001f);
    t.reset();
    assert(t.spikes() == 0 && t.rollingPeakPercent() == 0.f);
    assert(t.averagePercent() == 0 && t.maximumPercent() == 0);
    // Any counted spike must have a matching historical peak >= threshold.
    smoothride::PublishedDSPTelemetry published;
    t.reset();
    published.publish(t);
    for (int i = 0; i < 200; ++i) {
        t.record(i % 2 ? 0.000126 : 0.00112, 480, 48000, 2000 + i / 10, 10.0);
        published.publish(t);
        const auto snap = published.read();
        published.remember(snap);
        assert(std::abs(snap.peak - snap.historicalPeakEvent.block) < 0.0001f);
        if (snap.spikes > 0) {
            assert(snap.peak >= 10.f);
            assert(snap.lastSpikePercent >= 10.f);
            assert(snap.lastSpikeUs > 0.f);
            assert(std::abs(snap.lastSpikeBudgetUs - 10000.f) < 0.1f);
        }
    }
    t.reset(); published.publish(t);
    const auto cleared = published.read();
    assert(cleared.spikes == 0 && cleared.peak == 0 && cleared.lastSpikePercent == 0);
    // Dedicated guard-toggle timing must be recorded without changing spike semantics.
    t.reset();
    t.record(0.0002, 480, 48000, 3000, 10.0, false, 0.00009);
    assert(t.guardToggles() == 0);
    t.record(0.0015, 480, 48000, 3001, 10.0, true, 0.0003);
    assert(t.guardToggles() == 1 && t.spikes() == 1);
    assert(std::abs(t.lastTogglePercent() - 15.f) < 0.0001f);
    assert(std::abs(t.lastToggleRiderPercent() - 3.f) < 0.0001f);
    published.publish(t);
    const auto toggleSnapshot = published.read();
    assert(toggleSnapshot.guardToggles == 1);
    assert(std::abs(toggleSnapshot.lastTogglePercent - 15.f) < 0.0001f);
    assert(std::abs(toggleSnapshot.lastToggleRiderPercent - 3.f) < 0.0001f);
    t.record(0.0004, 480, 48000, 3002, 10.0, false, 0.0001);
    assert(t.guardToggles() == 1 && t.spikes() == 1);
    t.reset(); published.publish(t);
    assert(published.read().guardToggles == 0);
    // A synthetic toggle callback is accounted as PRE + RIDER + POST.
    // Monitor work is from the previous block and remains separate from BLOCK.
    t.record(0.0040, 480, 48000, 4000, 10.0, true,
             0.0007, 0.0012, 0.0021, 0.00015);
    published.publish(t);
    const auto phases = published.read();
    assert(phases.guardToggles == 1 && phases.spikes == 1);
    assert(std::abs(phases.lastTogglePercent - 40.f) < 0.001f);
    assert(std::abs(phases.lastTogglePrePercent - 12.f) < 0.001f);
    assert(std::abs(phases.lastToggleRiderPercent - 7.f) < 0.001f);
    assert(std::abs(phases.lastTogglePostPercent - 21.f) < 0.001f);
    assert(std::abs(phases.monitorUs - 150.f) < 0.01f);
    assert(std::abs(phases.lastTogglePrePercent + phases.lastToggleRiderPercent
        + phases.lastTogglePostPercent - phases.lastTogglePercent) < 0.001f);
    t.reset(); published.publish(t);
    assert(published.read().lastTogglePostPercent == 0.f);
    assert(published.read().monitorUs == 0.f);
    // v0.6.6: phase timing isolates parameters, input scan and A/B preparation.
    t.reset();
    t.record(0.0005, 480, 48000, 5000, 10.0, false, 0.0001,
             0.0003, 0.0001, 0.0, 0.00005, 0.0002, 0.00005, false);
    assert(std::abs(t.normalPreAveragePercent() - 3.f) < 0.001f);
    // Toggle callback: total 30%, PRE 20%, rider 7%, post 3%.
    t.record(0.003, 480, 48000, 5001, 10.0, true, 0.0007,
             0.002, 0.0003, 0.0, 0.0002, 0.0004, 0.0014, true);
    published.publish(t);
    auto detailed = published.read();
    assert(std::abs(detailed.lastToggleParamsPercent - 2.f) < 0.001f);
    assert(std::abs(detailed.lastToggleInputPercent - 4.f) < 0.001f);
    assert(std::abs(detailed.lastToggleABPercent - 14.f) < 0.001f);
    assert(detailed.lastToggleABRunning);
    assert(std::abs(detailed.lastToggleParamsPercent + detailed.lastToggleInputPercent
         + detailed.lastToggleABPercent - detailed.lastTogglePrePercent) < 0.001f);
    assert(std::abs(detailed.normalPreAveragePercent - 3.f) < 0.001f);
    // Non-toggle callback should update normal PRE but retain last-toggle metrics.
    t.record(0.0007, 480, 48000, 5002, 10.0, false, 0.0001,
             0.0004, 0.0002, 0.0, 0.00005, 0.00020, 0.00015, false);
    published.publish(t);
    detailed = published.read();
    assert(detailed.guardToggles == 1);
    assert(detailed.normalPreAveragePercent > 3.f);
    t.reset(); published.publish(t);
    detailed = published.read();
    assert(detailed.lastToggleABPercent == 0.f);
    assert(!detailed.lastToggleABRunning);
    assert(detailed.normalPreAveragePercent == 0.f);
    // v0.6.7: reset timing is separate from measured callback load.
    t.reset();
    t.record(0.001, 480, 48000, 6000, 10.0, false, 0.0001,
             0.0006, 0.0003, 0.0, 0.0002, 0.0003, 0.0001, false, 0.0005);
    published.publish(t);
    auto x = published.read();
    assert(std::abs(x.resetMicroseconds - 500.f) < 0.01f);
    assert(std::abs(x.normalParamsPercent - 2.f) < 0.001f);
    assert(std::abs(x.peak - 10.f) < 0.001f); // reset duration not included
    t.record(0.002, 480, 48000, 6001, 10.0, true, 0.0002,
             0.0016, 0.0002, 0.0, 0.0012, 0.0002, 0.0002, false);
    t.record(0.003, 480, 48000, 6002, 10.0, true, 0.0003,
             0.0024, 0.0003, 0.0, 0.0020, 0.0002, 0.0002, false);
    published.publish(t);
    x = published.read();
    assert(std::abs(x.lastTogglePercent - 30.f) < 0.001f);
    assert(std::abs(x.toggleParamsMeanPercent - 16.f) < 0.001f);
    assert(std::abs(x.toggleParamsMaxPercent - 20.f) < 0.001f);
    assert(std::abs(x.normalParamsPercent - 2.f) < 0.001f);
    t.reset(); published.publish(t);
    x = published.read();
    assert(x.toggleParamsMaxPercent == 0.f && x.normalParamsPercent == 0.f && x.resetMicroseconds == 0.f);
    std::cout << "Performance telemetry tests passed\n";
}
