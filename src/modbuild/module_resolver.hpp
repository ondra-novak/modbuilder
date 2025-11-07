#pragma once
#include <vector>
#include <filesystem>

class ModuleSourceScanner {
public:

    struct ModulePrefixMap {
        std::string prefix;
        std::vector<std::filesystem::path> paths;
    };

    struct Result {
        std::vector<std::filesystem::path> files;
        std::vector<ModulePrefixMap> mapping;                
    };


    static Result loadMap(std::filesystem::path directory);

};
