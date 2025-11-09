#include "module_resolver.hpp"
#include <fstream>
#include <json/parser.h>
#include <json/value.h>
#include "utils/log.hpp"
#include "utils/filesystem.hpp"


std::string_view ModuleResolver::modules_json = "modules.json";

ModuleResolver::Result scan_directory(const std::filesystem::path &directory) {

    ModuleResolver::Result out;

    std::error_code ec;
    if (ec == std::error_code{}) {

        std::filesystem::directory_iterator iter(directory), end{};
        while (iter != end) {
            const auto &entry = *iter;
            if (entry.is_regular_file()) {
                auto p = entry.path();
                auto ext = p.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [&](char c)->char{ return static_cast<char>(std::tolower(c));});
                if (ext == ".cpp" || ext == ".cppm" || ext == ".ixx") {
                    out.files.push_back(std::move(p));
                }            
            }
            ++iter;
        }
        out.origin = directory;
    } else {
        Log::warning("Error open directory {} - code:  {}", directory, ec.message());
    }

    return out;
}

ModuleResolver::Result process_json(const std::filesystem::path &directory, const json::value &js, std::filesystem::path origin) {
    
    ModuleResolver::Result result;
    result.origin = origin;

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
            ModuleResolver::ModulePrefixMap mp;
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


ModuleResolver::Result ModuleResolver::loadMap(std::filesystem::path directory)
{
    
    auto json_file = directory;
    
    if (!std::filesystem::is_regular_file(json_file)) {
        json_file = directory/modules_json;
    }

    std::ifstream jsin(json_file);

    if (jsin.is_open()) {


        Log::debug("Reading {}", json_file);

        try {
            std::string data(std::istreambuf_iterator<char>{jsin}, std::istreambuf_iterator<char>{});            
            return process_json(directory, json::value::from_json(data), json_file);
        } catch (std::exception &e) {
            Log::error("Failed to parse {} - exception {}", json_file, e.what());
            return {};
        }
    } 

    Log::debug("Can't open {}. Scanning whole directory {}", json_file.filename(), directory);

    return scan_directory(directory);



}

bool ModuleResolver::detect_change(std::filesystem::path directory, std::filesystem::file_time_type treshold) {
    std::error_code ec;
    auto wrtm = std::filesystem::last_write_time(directory/modules_json, ec);
    if (ec != std::error_code{}) {
        wrtm = std::filesystem::last_write_time(directory, ec);
        if (ec != std::error_code{}) {   
            return false;
        }        
    }
    return wrtm > treshold;
}

bool ModuleResolver::match_prefix(std::string_view prefix,
                                       std::string_view name) {

    if (prefix.empty()) return true;
    if (prefix.back() == '%') return name.compare(0, prefix.length()-1, prefix) == 0;
    if (prefix == name) return true;
    if (prefix.length() < name.length() && name.compare(0,prefix.length(), prefix) == 0) {
        return name[prefix.length()] == '.';
    }

  return false;
}
