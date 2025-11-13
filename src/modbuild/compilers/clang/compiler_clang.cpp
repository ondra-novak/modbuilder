#include "compiler_clang.hpp"
#include "factory.hpp"
#include "module_type.hpp"
#include <filesystem>

#include <regex>
#include <stdexcept>
#include <utils/arguments.hpp>
#include <utils/process.hpp>
#include <utils/temp_file.hpp>
#include <memory>

CompilerClang::CompilerClang(Config config) :_config((std::move(config))) {

    _module_cache = _config.working_directory / "pcm";
    _object_cache = _config.working_directory / "obj";
    std::filesystem::create_directories(_module_cache);
    std::filesystem::create_directories(_object_cache);

    ArgumentString verflg(version_flag);
    auto p = Process::spawn(_config.program_path, _config.working_directory, std::span<const ArgumentString>(&verflg,1), false);

    std::string banner((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());
    if (p.waitpid_status() != 0) {
        throw std::runtime_error("CLANG: unable to get version of compiler: " + _config.program_path.string());
    }

    // Extract version from banner (e.g., "clang version 14.0.6")
    std::regex version_regex(R"((\d+\.\d+(?:\.\d+)?))");
    std::smatch match;
    if (std::regex_search(banner, match, version_regex)) {
        _version = match[1].str();
    } else {
        throw std::runtime_error("CLANG: unable to parse version from: " + banner);
    }

    if (_version < Version("18.0")) {
        throw std::runtime_error("CLANG: version 18.0 or higher is required. Found: " + _version.to_string());
    }

}

int CompilerClang::link(std::span<const std::filesystem::path> objects) const {
    std::vector<ArgumentString> args = _config.link_options;
    std::transform(objects.begin(), objects.end(), std::back_inserter(args), [](const auto &p){
        return path_arg(p);
    });
    return invoke(_config, _config.working_directory, args);

}

std::string CompilerClang::preproces(const OriginEnv &env,const std::filesystem::path &file) const {

    auto args = prepare_args(env);
    args.insert(args.begin(), _config.compile_options.begin(), _config.compile_options.end());
    args.emplace_back(preprocess_flag);
    args.emplace_back(path_arg(file));
    
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
    if (precompile_stage && (source.type != ModuleType::implementation && source.type != ModuleType::partition )) {
        return args;
    }
    args = prepare_args(env);                                            
    args.insert(args.begin(), _config.compile_options.begin(), _config.compile_options.end());

    args.emplace_back(fprebuild_module_path);
    args.back().append(path_arg(_module_cache));

    for (const auto &m: modules) {
        if (m.type != ModuleType::system_header 
            && m.type != ModuleType::user_header
            && m.path.parent_path() == _module_cache) {
                break;       //skip modules in cache, not need specify
            }
        ArgumentString cmd (fmodule_file);
        cmd.append(string_arg(m.name));
        cmd.append(inline_arg("="));
        cmd.append(path_arg(m.path));
        args.push_back(std::move(cmd));        
    }

    switch (source.type) {

        case ModuleType::user_header: {
            result.interface = get_hdr_bmi_path(source);
            args.emplace_back(fmodule_header_user);
            args.emplace_back(xcpp_header);
            args.emplace_back(path_arg(source.path));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            return args;
        }
        case ModuleType::system_header: {
            result.interface = get_hdr_bmi_path(source);
            ensure_path_exists(result.interface);
            args.emplace_back(fmodule_header_system);        
            args.emplace_back(xcpp_system_header);
            args.emplace_back(path_arg(source.path));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            return args;
        }
        case ModuleType::partition:
        case ModuleType::interface: if (precompile_stage) {
            result.interface = get_bmi_path(source);
            ensure_path_exists(result.interface);
            args.emplace_back(xcpp_module);
            args.emplace_back(precompile_flag);
            args.emplace_back(path_arg(source.path));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            return args;
        }
        
        default: break;    
    }

    {
        result.object = get_obj_path(source);
        ensure_path_exists(result.object);
        args.emplace_back(compile_flag);
        args.emplace_back(path_arg(source.path));
        args.emplace_back(output_flag);
        args.emplace_back(path_arg(result.object));
        return args;
    }


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
