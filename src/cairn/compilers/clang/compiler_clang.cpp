#include "compiler_clang.hpp"
#include "factory.hpp"
#include "../../utils/log.hpp"
#include "module_type.hpp"
#include <algorithm>
#include <filesystem>

#include <regex>
#include <stdexcept>
#include <utils/arguments.hpp>
#include <utils/process.hpp>
#include <utils/temp_file.hpp>
#include <memory>

Version CompilerClang::get_clang_version(Config &cfg) {
    std::vector<ArgumentString> args;
    append_arguments(args, {"--version"},{});
    auto p = Process::spawn(cfg.program_path, cfg.working_directory, args, false);

    std::string banner((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());
    if (p.waitpid_status() != 0) {
        throw std::runtime_error("CLANG: unable to get version of compiler: " + cfg.program_path.string());
    }

    // Extract version from banner (e.g., "clang version 14.0.6")
    std::regex version_regex(R"((\d+\.\d+(?:\.\d+)?))");
    std::smatch match;
    if (std::regex_search(banner, match, version_regex)) {
        return match[1].str();
    } else {
        throw std::runtime_error("CLANG: unable to parse version from: " + banner);
    }


}

CompilerClang::CompilerClang(Config config) :_config((std::move(config))) {

    _module_cache = _config.working_directory / "pcm";
    _object_cache = _config.working_directory / "obj";
    std::filesystem::create_directories(_module_cache);
    std::filesystem::create_directories(_object_cache);

    _version  = get_clang_version(_config);

    if (_version < Version("18.0")) {
        throw std::runtime_error("CLANG: version 18.0 or higher is required. Found: " + _version.to_string());
    }
    
    if (std::find_if(_config.compile_options.begin(), _config.compile_options.end(), [&](const auto &opt){
        return opt.starts_with(stdcpp);
    }) == _config.compile_options.end()) {
        Log::warning("Missing C++ version (-std=c++xx) in command line options. This can cause misbehaviour during compilation!");
    }

}

int CompilerClang::link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const {
    std::vector<ArgumentString> args = _config.link_options;
    std::transform(objects.begin(), objects.end(), std::back_inserter(args), [](const auto &p){
        return path_arg(p);
    });
    append_arguments(args, {"-o","{}"}, {path_arg(target)});
    return invoke(_config, _config.working_directory, args);

}

SourceScanner::Info CompilerClang::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    return SourceScanner::scan_string(preprocess(env, file));
}

std::string CompilerClang::preprocess(const OriginEnv &env,const std::filesystem::path &file) const {

    auto args = prepare_args(env);
    args.insert(args.begin(), _config.compile_options.begin(), _config.compile_options.end());

    args.erase(std::remove_if(args.begin(), args.end(), [&,skip = false](const ArgumentString &s) mutable {
        if (skip) {
            skip = false;
            return false;
        }
        auto f1 = std::find(all_preproc.begin(), all_preproc.end(), s);
        if (f1 != all_preproc.end()) {
            skip = true;
            return false;
        }
        auto f2 = std::find_if(all_preproc.begin(), all_preproc.end(), [&](const ArgumentStringView &w) {
            return s.starts_with(w);
        });
        return (f2 == all_preproc.end());
    }), args.end());
    append_arguments(args, {"-xc++", "-E", "{}"}, {path_arg(file)});


    Process p = Process::spawn(_config.program_path, _config.working_directory, std::move(args), false);

    std::string out((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());

    p.waitpid_status();
    return out;
}

std::vector<ArgumentString> CompilerClang::build_arguments(bool precompile_stage,  const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const {

    std::vector<ArgumentString> args;
    if (!precompile_stage && (source.type == ModuleType::system_header || source.type == ModuleType::user_header )) {
        return args;
    }
    if (precompile_stage && (source.type == ModuleType::implementation || source.type == ModuleType::source )) {
        return args;
    }
    args = prepare_args(env);                                            
    args.insert(args.begin(), _config.compile_options.begin(), _config.compile_options.end());


    bool disable_experimental_warning = false;

    for (const auto &m: modules) {
        if (m.type != ModuleType::system_header 
            && m.type != ModuleType::user_header
            && m.path.parent_path() == _module_cache) {
                break;       //skip modules in cache, not need specify
            }
        append_arguments(args, {"-fmodule-file={}"}, {path_arg(m.path)});
        disable_experimental_warning = true;    
    }

    if (disable_experimental_warning) {
        append_arguments(args,{"-Wno-experimental-header-units"},{});
    }

    switch (source.type) {

        case ModuleType::user_header: {
            result.interface = get_hdr_bmi_path(source);
            append_arguments(args,
                {"-fmodule-header=user", "-xc++-header", "--precompile", "{}", "-o", "{}"},
                {path_arg(source.path), path_arg(result.interface)});
            return args;
        }
        case ModuleType::system_header: {
            result.interface = get_hdr_bmi_path(source);
            append_arguments(args,
                {"-fmodule-header=system", "-xc++-system-header", "--precompile", "{}", "-o", "{}"},
                {path_arg(source.path), path_arg(result.interface)});
            return args;
        }
        case ModuleType::partition:
        case ModuleType::interface: if (precompile_stage) {
            result.interface = get_bmi_path(source);
            append_arguments(args,
                {"-fprebuilt-module-path={}", "-xc++-module", "--precompile", "{}", "-o", "{}"},
                {path_arg(_module_cache), path_arg(source.path), path_arg(result.interface)});
            return args;
        }       
        default: break;    
    }

    result.object = get_obj_path(source);
    append_arguments(args,
        {"-fprebuilt-module-path={}","-c","{}","-o","{}"},
        {path_arg(_module_cache), path_arg(source.path), path_arg(result.object)});
    return args;

}


int CompilerClang::compile(const OriginEnv &env, 
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const {
    
    {
        auto args = build_arguments(true, env, source, modules, result);
        if (!args.empty()) {
            int r = invoke(_config, env.working_dir, args);
            if (r) return r;
        }
    }
    {
        auto args = build_arguments(false, env, source, modules, result);
        if (!args.empty()) {
            int r = invoke(_config, env.working_dir, args);
            if (r) return r;
        }
    }
    return 0;
    
}

bool CompilerClang::generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        std::vector<ArgumentString> &result) const {

    CompileResult dummy;
    result = build_arguments(false, env, src, modules, dummy);
    if (result.empty()) return false;
    result.insert(result.begin(), path_arg(_config.program_path));
    return true;

}


std::unique_ptr<AbstractCompiler> create_compiler_clang(AbstractCompiler::Config cfg) {
    return std::make_unique<CompilerClang>(std::move(cfg));
}
