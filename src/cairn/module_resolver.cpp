module;
#include <fkyaml.hpp>

module cairn.module_resolver;
import <cctype>;
import <filesystem>;
import <fstream>;
import <stdexcept>;
import <string>;
import <string_view>;
import <system_error>;
import cairn.utils.hash;
import cairn.utils.log;
import cairn.utils.utf8;


std::string_view ModuleResolver::modules_yaml = "modules.yaml";

void calculate_hash(ModuleResolver::Result &result) {
    std::hash<std::filesystem::path> hasher_p;
    std::hash<std::string> hasher_s;
    std::size_t h = hasher_p(result.env.working_dir);;
    for (const auto &p: result.env.includes) h = hash_combine(h, hasher_p(p));
    for (const auto &p: result.env.options) h = hash_combine(h, hasher_s(p));
    result.env.settings_hash = h;

}

ModuleResolver::Result scan_directory(const std::filesystem::path &directory) {

    Log::debug("Scanning directory {}", [&]{return directory.string();});

    ModuleResolver::Result out;

    std::error_code ec;
    if (ec == std::error_code{}) {

        std::filesystem::directory_iterator iter(directory), end{};
        while (iter != end) {
            const auto &entry = *iter;
            auto p = entry.path();
            if (entry.is_regular_file()) {
                auto ext = p.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [&](char c)->char{ return static_cast<char>(std::tolower(c));});
                if (ext == ".cpp" || ext == ".cppm" || ext == ".ixx") {
                    out.files.push_back(std::move(p));
                }            
            } else if (entry.is_directory()) {
                auto name = p.filename();
                out.env.maps.push_back({name.string(), {p}});
            }
            ++iter;
        }
        out.env.config_file = directory;
        out.env.working_dir = directory;
        calculate_hash(out);
    } else {
        Log::warning("Error open directory {} - code:  {}", directory.string(), ec.message());
    }

    return out;
}
ModuleResolver::Result process_yaml(const std::filesystem::path &yaml_file) {
    try {
        ModuleResolver::Result result;    
        result.env.config_file = yaml_file;
        result.env.working_dir = yaml_file.parent_path();
        
        std::ifstream f(yaml_file);
        if (!f) throw std::runtime_error("Can't open YAML file:" + yaml_file.string());
        fkyaml::node root = fkyaml::node::deserialize(f);

        auto files = root["files"];
        auto includes = root["includes"];
        auto options = root["options"];
        auto prefixes = root["prefixes"];
        auto work_dir = root["work_dir"];
        auto targets = root["targets"];

        if (!work_dir.is_null()) {
            if (!work_dir.is_string()) throw std::runtime_error("`work_dir` must be a path");
            result.env.working_dir = (result.env.working_dir/u8_from_string(work_dir.as_str())).lexically_normal();        
        }
        const auto &base = result.env.working_dir;

        if (!files.is_null()) {
            if (!files.is_sequence()) throw std::runtime_error("`includes` must be a sequence");
            for ( auto &x:files.as_seq()) {
                result.files.push_back((base/u8_from_string(x.as_str())).lexically_normal());
            }
        } else {
            auto r = scan_directory(base);
            result.files = std::move(r.files);
        }

        if (!includes.is_null()) {
            if (!includes.is_sequence()) throw std::runtime_error("`includes` must be a sequence");
            for (auto &x: includes.as_seq()) {
                result.env.includes.push_back((base/u8_from_string(x.as_str())).lexically_normal());
            }
        }

        if (!prefixes.is_null()) {
            if (!prefixes.is_mapping()) throw std::runtime_error("`prefixes` must be a key-value mapping");
            for ( auto &[k,v]: prefixes.as_map()) {            
                ModuleMapItem mitm{ k.as_str(), {}};
                if (v.is_sequence()) {
                    for (auto &x: v.as_seq()) {
                        mitm.paths.push_back((base/u8_from_string(x.as_str())).lexically_normal());
                    }
                } else {
                    mitm.paths.push_back((base/u8_from_string(v.as_str())).lexically_normal());
                }
                result.env.maps.push_back(mitm);
            }
        }

        if (!options.is_null()) {
            if (!options.is_sequence()) throw std::runtime_error("`options` must be a sequence");
                for ( auto &x:files.as_seq()) {
                    result.files.push_back(x.as_str());
                }
        }

        if (!targets.is_null()) {
            if (!targets.is_mapping()) throw std::runtime_error("`targets` must be a key-value mapping");
            for (auto &[k, v]: targets.as_map()) {
                result.targets.push_back({
                    (base/u8_from_string(k.as_str())).lexically_normal(),
                    (base/u8_from_string(v.as_str())).lexically_normal()
                });
            }
        }

        calculate_hash(result);
        return result;
    } catch (std::exception &e) {
        throw std::runtime_error("Failed to parse: "+yaml_file.string()+": " + e.what());
    }
}


ModuleResolver::Result ModuleResolver::loadMap(const std::filesystem::path &directory)
{
    
    std::error_code ec;
    std::filesystem::path cfg_file;
    if (std::filesystem::is_directory(directory, ec))  {
        cfg_file = directory/modules_yaml;
        if (!std::filesystem::is_regular_file(cfg_file, ec)) {
            return scan_directory(directory);
        }
    } else {
        cfg_file = directory;
    }
    Log::debug("Reading file {}", [&]{return cfg_file.string();});
    return process_yaml(cfg_file);



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
