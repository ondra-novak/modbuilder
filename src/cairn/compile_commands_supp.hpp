#pragma once

#include "utils/arguments.hpp"
#include "utils/hash.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>
#include "utils/nlohmann_json.hpp"

class CompileCommandsTable {
public:

    struct CCRecord {
        std::filesystem::path directory;
        std::filesystem::path file;
        std::vector<ArgumentString> arguments;
        ArgumentString command;
        std::filesystem::path output;
        nlohmann::json original_json  = {};
    };

    struct Key {
        std::filesystem::path source;
        std::filesystem::path output;
        bool operator==(const Key &other) const = default;
        std::size_t hash() const {
            std::hash<std::filesystem::path> hasher;
            return hash_combine(hasher(source), hasher(output));
        }
    };

    std::unordered_map<Key, CCRecord, MethodHash> _table;

    static CCRecord record(std::filesystem::path directory, 
        std::filesystem::path file, 
        std::vector<ArgumentString> arguments,
        std::filesystem::path output
    );

    static CCRecord record(std::filesystem::path directory, 
        std::filesystem::path file, 
        std::filesystem::path compiler,
        std::vector<ArgumentString> arguments,
        std::filesystem::path output
    );


    void load(std::filesystem::path p);
    void save(std::filesystem::path p);
    nlohmann::json export_db();
    void update(CCRecord rec);
    

};