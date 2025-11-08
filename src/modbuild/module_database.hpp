#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>


class ModuleDatabse {
public:

    struct Source {
        std::filesystem::path source_path;
        std::filesystem::path object_path;
        std::vector<std::string> requires_modules;
    };

    struct ModuleInfo {
        Source interface_file;
        std::vector<Source> implementation_units;
        std::filesystem::path bmi_file;    
    };

    using ModuleName = std::string;
    using ModuleMap = std::unordered_map<ModuleName, ModuleInfo>;
};