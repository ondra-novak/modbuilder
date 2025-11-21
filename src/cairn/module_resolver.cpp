#include "module_resolver.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <json/parser.h>
#include <json/value.h>
#include <string>
#include <string_view>
#include "utils/hash.hpp"
#include "utils/log.hpp"
#include "utils/filesystem.hpp"
#include "utils/fkyaml.hpp"


std::string_view ModuleResolver::modules_json = "modules.json";
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
        for (const auto &pfx: prefixes) {
            ModuleMapItem mp;
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
            result.env.maps.push_back(std::move(mp));
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


enum class YamlParseStage {
    none,
    files,
    includes,
    options,
    prefixes,
    prefix_value    
};

static std::string_view trim(std::string_view lw) {
        while (!lw.empty() && std::isspace(lw.front())) lw = lw.substr(1);
        while (!lw.empty() && std::isspace(lw.back())) lw = lw.substr(0,lw.size()-1);
        return lw;
}

static std::string_view decode_yaml_string(std::string_view str, std::string &buff) {
    buff.clear();
    char q = str.front();
    char prev = 0;
    str = str.substr(1, str.length()-2);
    for (char c : str) {
        if (prev == q) {
            buff.push_back(c);
            prev = 0;
        } else if (prev == '\\') {
            switch (c) {
                case 'r': buff.push_back('\r');break;
                case 'n': buff.push_back('\n');break;
                case 't': buff.push_back('\t');break;
                case 'a': buff.push_back('\a');break;
                default: buff.push_back(c);
            }
            prev = 0;
        } else {
            if (c == q || c=='\\') {
                prev = c;
            }  else {
                buff.push_back(c);
            }
        }
    }
    return buff;
}

static std::pair<std::string_view, std::string_view> split_on_colon(std::string_view s) {
    char q = 0;
    bool esc = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (q == 0) {
            if (c == '"' || c == '\'') {
                q = c;
                continue;
            }
            if (c == ':') {
                auto left = trim(s.substr(0, i));
                auto right = trim(s.substr(i + 1));
                return {left, right};
            }
        } else {
            if (esc) {
                esc = false;
                continue;
            }
            if (c == '\\') {
                esc = true;
                continue;
            }
            if (c == q) {
                q = 0;
            }
        }
    }
    return {trim(s), std::string_view{}};
}

ModuleResolver::Result process_yaml(std::filesystem::path yaml_file) {
    std::ifstream f(yaml_file);
    if (!f) throw std::runtime_error("Can't open YAML file");
    fkyaml::node root = fkyaml::node::deserialize(f);
    auto files = root["files"];
    auto includes = root["includes"];
    auto options = root["options"];
    auto prefixes = root["prefixes"];
    auto work_dir = root["work_dir"];







}

ModuleResolver::Result process_yaml_like(std::istream &file, std::filesystem::path origin) {
    std::string line;
    YamlParseStage stage = YamlParseStage::none;
    std::string cur_key;
    std::string decoded_string;
    std::string decoded_string2;
    ModuleResolver::Result res;
    auto dir = origin.parent_path();

    try {

        while (!!file) {
            std::getline(file, line);
            auto ln = trim(line);
            if (ln.empty()) continue;
            bool is_list = ln.front() == '-';
            if (is_list) {
                ln =  trim(ln.substr(1));
            }

            std::string_view mainp;
            std::string_view secp;

            if (!is_list) {
                auto [k,v] = split_on_colon(ln);            
                mainp = k;
                secp = v;
            } else {
                mainp = ln;
                secp = "";
            }

            bool has_ident = std::isspace(line.front());

            if (mainp.size()>1 && (mainp.front() == '"' || mainp.front() == '\'')
                && mainp.back() == ln.front()) {
                    mainp = decode_yaml_string(mainp, decoded_string);
            }
            if (secp.size()>1 && (secp.front() == '"' || secp.front() == '\'')
                && secp.back() == secp.front()) {
                    secp = decode_yaml_string(secp, decoded_string2);
            }
            if (is_list) {
                switch (stage) {
                    case YamlParseStage::files: 
                        res.files.push_back((dir/mainp).lexically_normal());
                        break;
                    case YamlParseStage::includes:
                        res.env.includes.push_back((dir/mainp).lexically_normal());
                        break;
                    case YamlParseStage::options:
                        res.env.options.emplace_back(mainp);
                        break;
                    case YamlParseStage::prefix_value:
                        if (res.env.maps.empty() || res.env.maps.back().prefix != cur_key) {
                            res.env.maps.push_back({cur_key,{}});
                        }
                        res.env.maps.back().paths.emplace_back((dir/mainp).lexically_normal());
                        break;
                    default:
                        throw ln;
                }
            } else {
                if (has_ident) {
                    if (stage == YamlParseStage::prefixes || stage == YamlParseStage::prefix_value) {
                        cur_key = mainp;
                        stage = YamlParseStage::prefix_value;
                    }
                } else {
                    if (mainp == "files") stage = YamlParseStage::files;
                    else if (mainp == "includes") stage = YamlParseStage::includes;
                    else if (mainp == "options") stage = YamlParseStage::options;
                    else if (mainp == "prefixes") stage = YamlParseStage::prefixes;
                    else if (mainp == "work_dir") res.env.working_dir = (dir/secp).lexically_normal();
                    else throw ln;

                }
            }
        }
    } catch (std::string_view ln) {
        Log::error("YAML: Don't understant this line: {}", ln);
        Log::warning("This YAML parser doesn't support all YAML features. Please keep recommended format");        
    }
    calculate_hash(res);
    res.env.config_file = origin;
    return res;
}


ModuleResolver::Result ModuleResolver::loadMap(const std::filesystem::path &directory)
{
    
    auto json_file = directory;
    
    if (!std::filesystem::is_regular_file(json_file)) {
        json_file = directory/modules_json;
        if (!std::filesystem::is_regular_file(json_file)) {
            json_file = directory/modules_yaml;
        }
    }


    std::ifstream jsin(json_file);

    if (jsin.is_open()) {

        Log::debug("Reading {}", json_file);

        auto ext = json_file.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](char c)->char{return static_cast<char>(std::tolower(c));});
        if (json_file.extension() == ".yaml") {
            return process_yaml_like(jsin,json_file);
        } else {
            try {
                std::string data(std::istreambuf_iterator<char>{jsin}, std::istreambuf_iterator<char>{});            
                return process_json(json_file, json::value::from_json(data), json_file);
            } catch (std::exception &e) {
                Log::error("Failed to parse {} - error: {}", json_file, e.what());
                return {};
            }
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
