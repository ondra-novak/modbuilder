#pragma once

#include <filesystem>
#include <vector>

struct ModuleMapItem {
    std::string prefix;
    std::vector<std::filesystem::path> paths;

    template<typename Me, typename Arch>
    static void serialize(Me &me, Arch &arch) {
        arch(me.prefix, me.paths);
    }


};
using ModuleMap = std::vector<ModuleMapItem>;

struct OriginEnv {
    std::filesystem::path config_file;             ///<path to origin 
    std::filesystem::path working_dir;      ///<path to working dir (for compiler)
    std::size_t settings_hash;              ///<hash of settings - compare to determine wether to recompile or not
    std::vector<std::filesystem::path> includes;    ///<list of additional includes
    std::vector<std::string> options;           ///list of other options
    ModuleMap maps;                           ///module maps

    static OriginEnv default_env() {
        auto cur = std::filesystem::current_path();
        return {
            cur, cur, 0, {}, {}, {}
        };
    }

    template<typename Me, typename Arch>
    static void serialize(Me &me, Arch &arch) {
        arch(me.config_file,me.working_dir,me.settings_hash,me.includes,me.options,me.maps);
    }

};

using POriginEnv = std::shared_ptr<OriginEnv>;