#pragma once
#include "origin_env.hpp"
#include <vector>
#include <filesystem>

class ModuleResolver {
public:

    struct ModulePrefixMap {
        std::string prefix;
        std::vector<std::filesystem::path> paths;
    };

    struct Result {
        std::vector<std::filesystem::path> files;
        std::vector<ModulePrefixMap> mapping;      
        OriginEnv env;
    };


    static Result loadMap(const std::filesystem::path &directory);
    static bool detect_change(const OriginEnv &env, std::filesystem::file_time_type treshold);

    static std::string_view modules_json;


    static bool match_prefix(std::string_view prefix, std::string_view name);
};
