#include "../Source/ParameterCache.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    using Cache = smoothride::ParameterCache;
    std::array<std::atomic<float>, Cache::count> values {};
    for (std::size_t i=0; i<values.size(); ++i) values[i].store(float(i), std::memory_order_relaxed);
    Cache cache;
    unsigned lookups = 0;
    bool bound = cache.bind([&](const char* name) -> std::atomic<float>* {
        ++lookups;
        for (std::size_t i=0; i<Cache::count; ++i)
            if (std::strcmp(Cache::ids[i], name) == 0) return &values[i];
        return nullptr;
    });
    assert(bound);
    assert(lookups == Cache::count);
    for (std::size_t i=0; i<Cache::count; ++i)
        assert(cache.get(static_cast<Cache::Index>(i)) == float(i));
    // Model host automation, preset load, and Noise Guard toggles: pointer binding
    // stays fixed while the underlying atomic value changes at any time.
    values[Cache::target].store(-18.5f);
    values[Cache::mix].store(82.0f);
    values[Cache::noiseGuard].store(1.0f);
    assert(cache.get(Cache::target) == -18.5f);
    assert(cache.get(Cache::mix) == 82.0f);
    assert(cache.get(Cache::noiseGuard) == 1.0f);
    values[Cache::noiseGuard].store(0.0f);
    assert(cache.get(Cache::noiseGuard) == 0.0f);
    assert(lookups == Cache::count); // No ID lookup while reading.
    Cache incomplete;
    assert(!incomplete.bind([](const char*) -> std::atomic<float>* { return nullptr; }));
    std::cout << "Parameter cache regression passed\n";
}
