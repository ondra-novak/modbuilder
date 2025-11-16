#include "module_resolver.hpp"
#include <fstream>
#include <json/parser.h>
#include <json/value.h>
#include "utils/hash.hpp"
#include "utils/log.hpp"
#include "utils/filesystem.hpp"


std::string_view ModuleResolver::modules_json = "modules.json";

void calculate_hash(ModuleResolver::Result &result) {
    std::hash<std::filesystem::path> hasher_p;
    std::hash<std::string> hasher_s;
    std::size_t h = hasher_p(result.env.working_dir);;
    for (const auto &p: result.env.includes) h = hash_combine(h, hasher_p(p));
    for (const auto &p: result.env.options) h = hash_combine(h, hasher_s(p));
    result.env.settings_hash = h;

}

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
        out.env.config_file = directory;
        out.env.working_dir = directory;
        calculate_hash(out);
    } else {
        Log::warning("Error open directory {} - code:  {}", directory, ec.message());
    }

    return out;
}

ModuleResolver::Result process_json(const std::filesystem::path &json_file, const json::value &js, std::filesystem::path origin) {
    
    ModuleResolver::Result result;
    result.env.config_file = origin;
    auto directory = json_file.parent_path();

    auto workdir = js["workdir"];
    if (workdir.type() == json::type::string) {
        directory =  std::filesystem::canonical(workdir.as<std::u8string>());
    }

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
    auto includes = js["includes"];
    if (includes.type() == json::type::array) {
        result.env.includes.reserve(includes.size());
        for (auto inc: includes) {
            result.env.includes.push_back(std::filesystem::canonical(directory/inc.as<std::u8string>()));
        }
    }
    auto options = js["options"];
    if (options.type() == json::type::array) {
        result.env.options.reserve(options.size());
        for (auto opt: options) result.env.options.push_back(opt.as<std::string>());
    }
    result.env.working_dir = std::move(directory);
    calculate_hash(result);
    return result;
}


ModuleResolver::Result ModuleResolver::loadMap(const std::filesystem::path &directory)
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
            return process_json(json_file, json::value::from_json(data), json_file);
        } catch (std::exception &e) {
            Log::error("Failed to parse {} - error: {}", json_file, e.what());
            return {};
        }
    } 

    Log::debug("Can't open {}. Scanning whole directory {}", json_file.filename(), directory);

    return scan_directory(directory);



}

bool ModuleResolver::detect_change(const OriginEnv &env, std::filesystem::file_time_type treshold) {
    std::error_code ec;    
    auto wrtm = std::filesystem::last_write_time(env.config_file, ec);
    if (ec != std::error_code{}) {
        return true;    //not exists? mark as changed
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
