#pragma once
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>

namespace smoothride {
// Fixed parameter order, matching the public APVTS IDs and preset snapshots.
// Bind once after APVTS construction; retain pointers, not sampled values.
// JUCE APVTS owns the atomic values throughout the processor lifetime.
class ParameterCache final {
public:
    enum Index : std::size_t {
        target, floor, range, maxCut, speed, downSpeed, mix, output,
        ceiling, gateHyst, lowProtect, adaptiveFloor, noiseGuard, count
    };
    static constexpr std::array<const char*, count> ids {{
        "target", "floor", "range", "maxCut", "speed", "downSpeed", "mix", "output",
        "ceiling", "gateHyst", "lowProtect", "adaptiveFloor", "noiseGuard"
    }};
    template <typename Lookup>
    bool bind(Lookup&& lookup) noexcept {
        std::array<std::atomic<float>*, count> resolved {};
        for (std::size_t i = 0; i < count; ++i) {
            resolved[i] = lookup(ids[i]);
            if (resolved[i] == nullptr) return false;
        }
        values = resolved;
        return true;
    }
    float get(Index index) const noexcept {
        auto* ptr = values[static_cast<std::size_t>(index)];
        assert(ptr != nullptr);
        return ptr->load(std::memory_order_relaxed);
    }
private:
    std::array<std::atomic<float>*, count> values {};
};
} // namespace smoothride
