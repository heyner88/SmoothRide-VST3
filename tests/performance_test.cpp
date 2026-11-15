#include "../Source/PerformanceTelemetry.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
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
    std::cout << "Performance telemetry tests passed\n";
}
