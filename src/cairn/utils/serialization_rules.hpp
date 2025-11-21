#pragma once


#include "serializer.hpp"
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

template<typename T, typename Alloc>
struct SerializationRule<std::vector<T, Alloc> > {

    template<typename Obj, typename Arch>
    static void serialize(Obj &v, Arch &arch) {
        if constexpr(std::is_const_v<std::remove_reference_t<Obj> >) {
            std::size_t sz  = v.size();
            arch(sz);
            for (const auto &x: v) arch(x);
        } else {
            std::size_t sz;
            arch(sz);
            v.resize(sz);
            for (auto &x: v) arch(x);

        }
    }
};


template<>
struct SerializationRule<std::filesystem::path> {

    template<typename Obj, typename Arch>
    static void serialize(Obj &v, Arch &arch) {
        if constexpr(std::is_const_v<std::remove_reference_t<Obj> >) {
            auto s = v.u8string();
            arch(s);
        } else {
            std::u8string s;
            arch(s);
            v = s;
        }
    }
};

template<typename T, typename Traits, typename Alloc>
struct SerializationRule<std::basic_string<T, Traits, Alloc> > {
    template<typename Obj, typename Arch>
    static void serialize(Obj &v, Arch &arch) {
        if constexpr(std::is_const_v<std::remove_reference_t<Obj> >) {
            std::size_t sz  = v.size();
            arch(sz);
            for (const auto &x: v) arch(x);
        } else {
            std::size_t sz;
            arch(sz);
            v.resize(sz);
            for (auto &x: v) arch(x);
        }
    }
};

template<typename T, typename V, typename Cmp, typename Alloc>
struct SerializationRule<std::map<T, V, Cmp, Alloc> > {

    template<typename Obj, typename Arch>
    static void serialize(Obj &v, Arch &arch) {
        if constexpr(std::is_const_v<std::remove_reference_t<Obj> >) {
            std::size_t sz  = v.size();
            arch(sz);
            for (const auto &[key,val]: v) arch(key,val);
        } else {
            std::size_t sz;
            arch(sz);
            for (std::size_t i=0;i<sz;++i) {
                T key;
                V value;
                arch(key, value);
                v.emplace(std::move(key), std::move(value));
            }
        }
    }
};

