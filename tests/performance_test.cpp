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
    t.reset();
    assert(t.averagePercent() == 0 && t.maximumPercent() == 0);
    std::cout << "Performance telemetry tests passed\n";
}
