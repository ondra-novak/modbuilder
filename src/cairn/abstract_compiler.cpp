#include "abstract_compiler.hpp"
#include "module_type.hpp"
#include "utils/process.hpp"
#include <charconv>
#include <filesystem>
#include <format>
#include <string_view>

int AbstractCompiler::invoke(const Config &cfg, 
    const std::filesystem::path &workdir, 
    std::span<const ArgumentString> arguments)
{
    Process p = Process::spawn(cfg.program_path, workdir, arguments, true);
    return p.waitpid_status();

}

std::filesystem::path AbstractCompiler::intermediate_file(const SourceDef &src, std::string_view ext) {

    if (ext.starts_with('.')) ext = ext.substr(1);
    
    std::hash<std::filesystem::path> path_hasher;
    std::size_t h1 = path_hasher(src.path.parent_path());
    std::string whole_name = std::format("{}_{:x}.{}", src.path.stem().string(), h1, ext);
    return whole_name;

}


std::vector<ArgumentString> AbstractCompiler::prepare_args(const OriginEnv &env) {
    std::vector<ArgumentString> out;
    ArgumentString a;
    for (const auto &i: env.includes) {
        auto s = path_arg(i);
        a.push_back('-');
        a.push_back('I');
        a.append(s);
        out.push_back(a);
    }
    return out;        
}
