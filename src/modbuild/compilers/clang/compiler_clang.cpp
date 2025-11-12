#include "compiler_clang.hpp"
#include "factory.hpp"
#include "module_type.hpp"
#include <filesystem>
#include <stdexcept>
#include <utils/product_name.hpp>
#include <utils/arguments.hpp>
#include <utils/process.hpp>
#include <utils/temp_file.hpp>
#include <memory>


std::vector<ArgumentString> CompilerClang::prepare_args(const OriginEnv &env) {
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


int CompilerClang::link(std::span<const std::filesystem::path> objects) const {
    std::vector<ArgumentString> args = _config.link_options;
    std::transform(objects.begin(), objects.end(), std::back_inserter(args), [](const auto &p){
        return path_arg(p);
    });
    return spawn_compiler(_config, _config.working_directory, args,nullptr);

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

std::unique_ptr<AbstractCompiler> CompilerClang::create(std::span<const ArgumentString> arguments,
        std::filesystem::path workdir
    ) {
    return std::make_unique<CompilerClang>(parse_commandline(arguments,workdir));
}

int CompilerClang::compile(const OriginEnv &env, const std::filesystem::path &source_ref, 
        ModuleType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const {
    
    auto args = prepare_args(env);
    args.insert(args.begin(), _config.compile_options.begin(), _config.compile_options.end());
    auto arglen = args.size();

    for (const auto &m: modules) {
        ArgumentString cmd (fmodule_file);
        cmd.append(string_arg(m.logical_name));
        cmd.append(inline_arg("="));
        cmd.append(path_arg(m.interface));
        args.push_back(std::move(cmd));        
    }

    switch (type) {

        case ModuleType::user_header: {
            result.interface = get_bmi_path(type, source_ref);
            ensure_path_exists(result.interface);
            args.emplace_back(fmodule_header_user);
            args.emplace_back(xcpp_header);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            return spawn_compiler(_config, env.working_dir, args);
        }
        case ModuleType::system_header: {
            result.interface = get_bmi_path(type, source_ref);
            ensure_path_exists(result.interface);
            args.emplace_back(fmodule_header_system);        
            args.emplace_back(xcpp_system_header);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            return spawn_compiler(_config, env.working_dir, args);
        }
        case ModuleType::partition:
        case ModuleType::interface: {
            result.interface = get_bmi_path(type, source_ref);
            ensure_path_exists(result.interface);
            args.emplace_back(xcpp_module);
            args.emplace_back(precompile_flag);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            int r =  spawn_compiler(_config, env.working_dir, args);
            if (r) return r;
            args.resize(arglen);
            break;
        }
        
        default: break;    
    }

    {
        result.object = get_obj_path(type, source_ref);
        ensure_path_exists(result.object);
        args.emplace_back(compile_flag);
        args.emplace_back(path_arg(source_ref));
        args.emplace_back(output_flag);
        args.emplace_back(path_arg(result.object));
        return  spawn_compiler(_config, env.working_dir, args, &result.compile_arguments);
    }

}

bool CompilerClang::generate_compile_command(const OriginEnv &env,
                                        const std::filesystem::path &source, 
                                        ModuleType type,
                                        std::span<const ModuleMapping> modules,
                                        std::vector<ArgumentString> &result) const {
    if (type == ModuleType::system_header || type == ModuleType::user_header) {
        return false;
    }
    result = prepare_args(env);
    result.insert(result.begin(), _config.compile_options.begin(), _config.compile_options.end());
    result.emplace_back(compile_flag);
    result.emplace_back(path_arg(source));
    result.emplace_back(output_flag);
    result.emplace_back(path_arg(get_obj_path(type, source)));
    return true;

}


std::unique_ptr<AbstractCompiler> create_compiler_clang(AbstractCompiler::Config cfg) {
    return std::make_unique<CompilerClang>(std::move(cfg));
}
