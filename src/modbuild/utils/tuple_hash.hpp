#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>   // std::index_sequence

// ------------------------------------------------------------
// hash_combine – the same mixing function used by Boost
// ------------------------------------------------------------
inline std::size_t hash_combine(std::size_t lhs, std::size_t rhs) noexcept
{
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
}

// ------------------------------------------------------------
// tuple_hash – works for any std::tuple<Ts...>
// ------------------------------------------------------------
template<template<class> class Hasher = std::hash>
struct tuple_hash {
    // generic case – any tuple size
    template <class... Ts>
    std::size_t operator()(const std::tuple<Ts...>& t) const noexcept
    {
        return std::apply([](const auto & ... x){
            std::size_t seed = 0;    
            ((seed = hash_combine(seed, (Hasher<std::remove_cvref_t<decltype(x)> > ())(x))), ...);
            return seed;
        }, t);
    }

};