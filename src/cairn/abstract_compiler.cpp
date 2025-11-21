#include "abstract_compiler.hpp"
#include "utils/process.hpp"
#include "utils/log.hpp"
#include <filesystem>
#include <format>
#include <string_view>

int AbstractCompiler::invoke(const Config &cfg, 
    const std::filesystem::path &workdir, 
    std::span<const ArgumentString> arguments)
{
    Process p = Process::spawn(cfg.program_path, workdir, arguments, Process::no_streams);
    return p.waitpid_status();

}

std::filesystem::path AbstractCompiler::intermediate_file(const SourceDef &src, std::string_view ext) {

    if (ext.starts_with('.')) ext = ext.substr(1);
    
    std::hash<std::filesystem::path> path_hasher;
    std::size_t h1 = path_hasher(src.path.parent_path());
    std::string whole_name = std::format("{}_{:x}.{}", src.path.stem().string(), h1, ext);
    return whole_name;

}


std::vector<ArgumentString> AbstractCompiler::prepare_args(const OriginEnv &env, const Config &config, char switch_char) {
    std::vector<ArgumentString> out;
    ArgumentString a;
    for (const auto &i: env.includes) {
        auto s = path_arg(i);
        a.clear();
        a.push_back(switch_char);
        a.push_back('I');
        a.append(s);
        out.push_back(a);
    }
    for (const auto &o: env.options) {
        out.push_back(string_arg(o));
    }
    for (const auto &c: config.compile_options) {
        out.push_back(c);
    }

    return out;        
}

void AbstractCompiler::dump_failed_cmdline(const Config &cfg, const std::filesystem::path &workdir, std::span<const ArgumentString> cmdline) {
    Log::error("Failed command: {}", [&]{
        std::ostringstream s;
        s << cfg.program_path.string();
        for (const auto &x: cmdline) {
            s << " " << std::filesystem::path(x);
        };
        return std::move(s).str();
    });
    Log::verbose("Working directory: {}", workdir.string());
}

std::filesystem::path AbstractCompiler::find_in_path(std::filesystem::path name, const SystemEnvironment &env)
{    
    auto lst_str =env["PATH"];
#ifdef _WIN32
    wchar_t sep = L';';
#else  
    wchar_t sep = ':';
#endif

    while (!lst_str.empty()) {
        auto n = lst_str.find(sep);
        auto p = n == lst_str.npos?lst_str:lst_str.substr(0,n);
        lst_str = n == lst_str.npos?decltype(lst_str)():lst_str.substr(n+1);
        std::filesystem::path dir(p);
        std::filesystem::path candidate = dir / name;
        if (std::filesystem::exists(candidate) &&
            std::filesystem::is_regular_file(candidate)) {
                return candidate;                
        }

    }

    if (!std::filesystem::is_regular_file(name)) {
        throw std::runtime_error("Unable to find executable: "+name.string());
    }
    return name;
}
