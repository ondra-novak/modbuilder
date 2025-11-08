#pragma once


#include "utils/arguments.hpp"
#include <cctype>
#include <charconv>
#include <filesystem>
#include "module_type.hpp"

inline ArgumentString product_name(ModuleReferenceType type, const std::filesystem::path &source, std::string_view ext) {
    auto path = path_arg(source.parent_path());
    std::hash<ArgumentString> hasher;
    auto hash = hasher(path);
    char hexbuff[50];
    std::to_chars(hexbuff, hexbuff+sizeof(hexbuff), hash, 16);
    ArgumentString ret = string_arg(hexbuff);
    ret.push_back('_');
    switch (type) {
        case ModuleReferenceType::header: ret.push_back('h');
                                          ret.push_back('_');
                                          break;
        case ModuleReferenceType::system_header: ret.push_back('s');
                                          ret.push_back('_');
                                          break;
        default:break;                                        
    }

    ret.append(path_arg(source.stem()));
    ret.push_back('.');
    ret.append(string_arg(ext));
    return ret;

}
