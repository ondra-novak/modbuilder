#pragma once

#include "utils/arguments.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <json/value.h>
class CompileCommandsTable {
public:

    struct CCRecord {
        std::filesystem::path directory;
        std::filesystem::path file;
        std::vector<ArgumentString> arguments;
        ArgumentString command;
        json::value original_json  = {};
    };

    std::unordered_map<std::filesystem::path, CCRecord> _table;

    static CCRecord record(std::filesystem::path directory, 
        std::filesystem::path file, 
        std::vector<ArgumentString> arguments);


    void load(std::filesystem::path p);
    void save(std::filesystem::path p);
    json::value export_db();
    void update(CCRecord rec);
    

};