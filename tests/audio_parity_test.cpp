// RC1 release gate: compare current gain rider against frozen, unmodified v0.9.4.
// This tests the audio engine directly, not the JUCE host wrapper.
#include "../Source/GainRider.h"
#define smoothride smoothride_reference
#include "reference/GainRider_v0_9_4.h"
#undef smoothride
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

constexpr double pi = 3.14159265358979323846;
static void check(bool ok, const char* reason) {
    if (!ok) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}

static std::vector<float> signal(int sampleCount, int sampleRate, int variant) {
    std::vector<float> result(sampleCount);
    uint32_t state = 0x12345678u;
    for (int i = 0; i < sampleCount; ++i) {
        state = state * 1664525u + 1013904223u;
        const double noise = (double(int(state >> 8)) / 8388608.0) - 1.0;
        const double t = double(i) / sampleRate;
        double v = 0.0;
        if (variant == 0) v = 0.25 * std::sin(2 * pi * 440.0 * t) + 0.04 * noise;
        if (variant == 1) v = (i / (sampleRate / 2) % 2 == 0 ? 0.0003 : 0.0) * (std::sin(2*pi*220*t) + 0.2*noise);
        if (variant == 2) v = 0.9 * std::sin(2*pi*70*t) + 0.18 * noise;
        if (variant == 3) v = (i / (sampleRate / 4) % 3 == 0 ? 0.00001 : 0.01) * std::sin(2*pi*110*t);
        result[i] = float(v);
    }
    return result;
}

static void runCase(int sampleRate, int variant, bool stereo, int chunk,
                    const smoothride::Settings& config, const char* label) {
    const int count = sampleRate * 2;
    auto left = signal(count, sampleRate, variant);
    auto right = signal(count, sampleRate, (variant + 1) % 4);
    auto leftBaseline = left, rightBaseline = right;
    smoothride::GainRider candidate;
    smoothride_reference::GainRider baseline;
    candidate.prepare(sampleRate);
    baseline.prepare(sampleRate);
    candidate.setSettings(config);
    smoothride_reference::Settings old;
    old.targetDb = config.targetDb;
    old.floorDb = config.floorDb;
    old.maxBoostDb = config.maxBoostDb;
    old.maxCutDb = config.maxCutDb;
    old.upMs = config.upMs;
    old.downMs = config.downMs;
    old.mix = config.mix;
    old.outputDb = config.outputDb;
    old.ceilingDb = config.ceilingDb;
    old.gateHysteresisDb = config.gateHysteresisDb;
    old.lowLevelProtect = config.lowLevelProtect;
    old.adaptiveFloor = config.adaptiveFloor;
    old.noiseGuard = config.noiseGuard;
    baseline.setSettings(old);
    for (int pos = 0; pos < count; pos += chunk) {
        const int n = std::min(chunk, count - pos);
        candidate.process(left.data() + pos, stereo ? right.data() + pos : nullptr, n, stereo);
        baseline.process(leftBaseline.data() + pos, stereo ? rightBaseline.data() + pos : nullptr, n, stereo);
        for (int j = 0; j < n; ++j) {
            const int k = pos + j;
            if (left[k] != leftBaseline[k] || (stereo && right[k] != rightBaseline[k])) {
                std::cerr << "Audio mismatch " << label << " sample " << k << '\n';
                std::exit(1);
            }
        }
        check(candidate.currentRideDb() == baseline.currentRideDb(), "gain telemetry differs");
        check(candidate.currentRequestedRideDb() == baseline.currentRequestedRideDb(), "requested ride differs");
        check(candidate.isRidingActive() == baseline.isRidingActive(), "gate differs");
        check(candidate.isNoiseBlocked() == baseline.isNoiseBlocked(), "noise guard differs");
    }
    std::cout << "PASS " << label << " " << sampleRate << " Hz "
              << (stereo ? "stereo" : "mono") << " block=" << chunk << '\n';
}

int main() {
    using smoothride::Settings;
    Settings standard;
    Settings adaptive = standard; adaptive.adaptiveFloor = 1;
    Settings guard = adaptive; guard.noiseGuard = 1; guard.upMs = 100;
    Settings soft = adaptive; soft.lowLevelProtect = 1; soft.gateHysteresisDb = 4;
    Settings dry = standard; dry.mix = 0; dry.outputDb = 0; dry.ceilingDb = -0.1f;
    Settings fullWet = standard; fullWet.mix = 1; fullWet.upMs = 100; fullWet.downMs = 50;
    for (int sr : {44100, 48000, 96000}) {
        runCase(sr, 0, true, 512, standard, "baseline music");
        runCase(sr, 1, true, 127, adaptive, "adaptive quiet/gaps");
        runCase(sr, 2, true, 4096, fullWet, "loud/cut/limiter");
        runCase(sr, 3, false, 257, soft, "soft floor mono");
        runCase(sr, 1, false, 512, guard, "noise guard mono");
        runCase(sr, 0, true, 1024, dry, "dry mix");
    }
    std::cout << "PASS v0.9.4 audio parity (sample-exact)\n";
}
