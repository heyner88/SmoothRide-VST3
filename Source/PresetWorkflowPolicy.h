#pragma once
#include <array>
#include <cmath>
#include <cstddef>

namespace smoothride::preset {
// Message-thread preset rules; independent of JUCE so they can be regression-tested.
inline constexpr std::size_t maxNameLength = 40;
inline constexpr float modificationTolerance = 0.02f;

inline bool validName(std::size_t trimmedCharacterCount) noexcept {
    return trimmedCharacterCount > 0 && trimmedCharacterCount <= maxNameLength;
}

inline bool mayCreate(std::size_t trimmedCharacterCount, bool alreadyExists) noexcept {
    return validName(trimmedCharacterCount) && !alreadyExists;
}

// Uses the same tolerance as the existing active-preset status indicator.
template <std::size_t N>
inline bool isModified(const std::array<float, N>& saved,
                       const std::array<float, N>& current) noexcept {
    for (std::size_t i = 0; i < N; ++i)
        if (!std::isfinite(saved[i]) || !std::isfinite(current[i]) ||
            std::abs(saved[i] - current[i]) > modificationTolerance)
            return true;
    return false;
}
}
