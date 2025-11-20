#pragma once

#include <string_view>

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

    ///Module partition includes both interface and implementation
    /**
    generates BMI and O
    but are considered as implementation
    but can be imported
     */
    partition,

    ///Ordinary source file
    /**
     Just a source file not declared as module, but still can import
    */
    source,

    ///Module is precompiled system header
    system_header,


    ///Module is precompiled user header
    user_header,
};

constexpr std::string_view to_string(ModuleType t) {
    switch (t) {
        case ModuleType::interface: return "interface";
        case ModuleType::implementation: return "implementation";
        case ModuleType::source: return "source";
        case ModuleType::partition: return "partition";
        case ModuleType::system_header: return "system_header";
        case ModuleType::user_header: return "user_header";
        default: return "unknown";
    }
}


constexpr bool generates_bmi(ModuleType t) {
    return t == ModuleType::user_header
        || t == ModuleType::system_header
        || t == ModuleType::partition
        || t == ModuleType::interface;
}
constexpr bool generates_object(ModuleType t) {
    return t == ModuleType::implementation
        || t == ModuleType::source
        || t == ModuleType::interface
        || t == ModuleType::partition;
}
constexpr bool is_header_module(ModuleType t) {
    return t == ModuleType::user_header || t == ModuleType::system_header;
}
