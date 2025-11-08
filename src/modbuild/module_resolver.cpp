#include "module_resolver.hpp"
#include <fstream>
#include <json/parser.h>
#include <json/value.h>
#include "utils/log.hpp"



ModuleSourceScanner::Result scan_directory(const std::filesystem::path &directory) {

    ModuleSourceScanner::Result out;

    std::filesystem::directory_iterator iter(directory), end{};
    while (iter != end) {
        const auto &entry = *iter;
        if (entry.is_regular_file()) {
            auto p = entry.path();
            auto ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [&](char c)->char{ return static_cast<char>(std::tolower(c));});
            if (ext == ".cpp" || ext == ".cppm") {
                out.files.push_back(std::move(p));
            }            
        }
    }

    return out;
}

ModuleSourceScanner::Result process_json(const std::filesystem::path &directory, const json::value &js) {
    
    ModuleSourceScanner::Result result;

    auto files = js["files"];
    if (files.type() == json::type::array) {
        for (auto itm: files) {
            if (itm.contains<std::u8string>()) {
                result.files.push_back(
                    (directory / itm.as<std::u8string_view>()).lexically_normal());
            }
        }
    }
    auto prefixes = js["prefixes"];
    if (prefixes.type() == json::type::object) {
        for (auto pfx: prefixes) {
            ModuleSourceScanner::ModulePrefixMap mp;
            mp.prefix = pfx.key();
            if (pfx.type() == json::type::string) {
                mp.paths.push_back((directory/pfx.as<std::u8string_view>()).lexically_normal());
            } else if (pfx.type() == json::type::array) {
                for (auto itm: files) {
                    if (itm.contains<std::u8string>()) {
                        mp.paths.push_back(
                            (directory / itm.as<std::u8string_view>()).lexically_normal());
                    }
                }

            }
            result.mapping.push_back(std::move(mp));
        }        
    }
    return result;
}


ModuleSourceScanner::Result ModuleSourceScanner::loadMap(std::filesystem::path directory)
{
    auto json_file = directory/"modules.json";

    std::fstream jsin(json_file);

    if (jsin.is_open()) {
        Log::debug("Reading {}", json_file.string());

        try {
            json::parser_t pr;
            json::value v;
            pr.parse(std::istreambuf_iterator<char>{jsin}, std::istreambuf_iterator<char>{}, v);
            return process_json(directory, v);
        } catch (std::exception &e) {
            Log::error("Failed to parse {} - exception {}", json_file.string(), e.what());
            return {};
        }
    } 

    Log::debug("Can't open {}. Scanning whole directory {}", json_file.filename().string(), directory.string());

    return scan_directory(directory);



}
