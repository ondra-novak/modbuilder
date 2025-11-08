#pragma once

#include "abstract_compiler.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>


#include "module_type.hpp"


class ModuleDatabse {
public:

    struct Source {
        std::filesystem::path source_path;
        std::filesystem::path object_path;
        std::vector<std::string> required_modules;
    };

    struct ModuleInfo {
        Source interface_file;
        std::vector<Source> implementation_units;
        std::filesystem::path bmi_file;    
    };

    using ModuleName = std::string;
    using ModuleMap = std::unordered_map<ModuleReferenceType, ModuleInfo, tuple_hash<Hasher> >;


    struct ScanResult {
        std::vector<ModuleName> unresolved;
    };

protected:

    ModuleMap _map;

    

};