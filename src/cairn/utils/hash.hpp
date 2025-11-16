#pragma once

#include <cstddef>
inline std::size_t hash_combine(std::size_t lhs, std::size_t rhs) noexcept
{
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}


struct MethodHash {
    constexpr std::size_t operator()(const auto &key) const {
        return key.hash();
    }
};
