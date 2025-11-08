#pragma once
#include <tuple>
#include "utils/tuple_hash.hpp"
#include <string>


enum class ModuleType {
    ///Interface module
    /**
     Interface module has "export module Name;"
    */
    interface,

    ///Implementation module
    /**
    Implementation module has "module Name;"
    */
    implementation,

    ///Ordinary source file
    /**
     Just a source file not declared as module, but still can import
    */
    source
};


enum class ModuleReferenceType {

    ///standard module
    module,

    ///system module (std and std.compat)
    system_module,

    ///header imported as module
    header,

    ///system header imported as module
    system_header,

    ///not module (used to reference way of compile)
    none

};

template<typename T>
struct Hasher;

template<>
struct Hasher<ModuleReferenceType> {
    std::size_t operator()(ModuleReferenceType r) const {
        return static_cast<int>(r);
    }
};

template<>
struct Hasher<std::string> : std::hash<std::string> {};

using ModuleReference = std::tuple<ModuleReferenceType, std::string>;

