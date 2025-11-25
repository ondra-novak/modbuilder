#pragma once
#include "compile_target.hpp"
#include "origin_env.hpp"
#include <vector>
#include <filesystem>

class ModuleResolver {
public:

    struct Result {
        std::vector<std::filesystem::path> files;
        std::vector<CompileTarget> targets;
        OriginEnv env;
    };


    static Result loadMap(const std::filesystem::path &directory);
    static bool detect_change(const OriginEnv &env, std::filesystem::file_time_type treshold);

    static std::string_view modules_yaml;


    static bool match_prefix(std::string_view prefix, std::string_view name);
};
