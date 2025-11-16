#pragma once

#include <filesystem>
#include <vector>
struct OriginEnv {
    std::filesystem::path config_file;             ///<path to origin 
    std::filesystem::path working_dir;      ///<path to working dir (for compiler)
    std::size_t settings_hash;              ///<hash of settings - compare to determine wether to recompile or not
    std::vector<std::filesystem::path> includes;    ///<list of additional includes
    std::vector<std::string> options;           ///list of other options

    static OriginEnv default_env() {
        auto cur = std::filesystem::current_path();
        return {
            cur, cur, 0, {}, {}
        };
    }


};

using POriginEnv = std::shared_ptr<OriginEnv>;