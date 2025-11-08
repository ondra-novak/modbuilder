#include "compiler_clang.hpp"
#include "module_type.hpp"
#include <filesystem>
#include <stdexcept>
#include <utils/product_name.hpp>
#include <utils/arguments.hpp>
#include <utils/process.hpp>
#include <utils/temp_file.hpp>
#include <memory>
#include <ostream>



int CompilerClang::link([[maybe_unused]] std::filesystem::path binary,
                            [[maybe_unused]] std::span<const std::filesystem::path> objects) const {
    throw std::runtime_error("not implemented");
}

std::string CompilerClang::preproces(const std::filesystem::path &file) const {

    std::vector<ArgumentString> args = _config.compile_options;
    args.emplace_back(preprocess_flag);
    args.emplace_back(path_arg(file));
    
    Process p = Process::spawn(_config.program_path, _config.working_directory, std::move(args));

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

int CompilerClang::compile(const std::filesystem::path &source_ref, 
        ModuleReferenceType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const {
    
    std::vector<ArgumentString> args = _config.compile_options;
    std::filesystem::path bmi_path = ".cache";
    std::filesystem::path obj_path = ".obj";

    std::filesystem::create_directories(bmi_path);
    std::filesystem::create_directories(obj_path);

    std::vector<ArgumentString> modules_args;

    for (const auto &m: modules) {
        ArgumentString cmd (fmodule_file);
        cmd.append(string_arg(m.logical_name));
        cmd.append(inline_arg("="));
        cmd.append(path_arg(m.interface));
        modules_args.push_back(std::move(cmd));        
    }


    switch (type) {

        case ModuleReferenceType::header: {
            result.interface = bmi_path/product_name(type, source_ref, "pcm");
            args.emplace_back(fmodule_header_user);
            args.emplace_back(xcpp_header);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            Process p = Process::spawn_noredir(_config.program_path,_config.working_directory, args);
            return p.waitpid_status();
        }
        case ModuleReferenceType::system_header: {
            result.interface = bmi_path/product_name(type, source_ref, "pcm");
            args.emplace_back(fmodule_header_system);        
            args.emplace_back(xcpp_system_header);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            Process p = Process::spawn_noredir(_config.program_path,_config.working_directory, args);
            return p.waitpid_status();
        }
        case ModuleReferenceType::module: {
            result.interface = bmi_path/product_name(type, source_ref,"pcm");
            args.emplace_back(precompile_flag);
            args.emplace_back(path_arg(source_ref));
            args.emplace_back(output_flag);
            args.emplace_back(path_arg(result.interface));
            args.insert(args.end(), modules_args.begin(), modules_args.end());
            Process p = Process::spawn_noredir(_config.program_path,_config.working_directory, args);
            int r =  p.waitpid_status();
            if (r) return r;
            break;
        }
        case ModuleReferenceType::system_module:
            throw std::runtime_error("not implemented yet");
        
        default: break;    
    }

    {
        result.object = obj_path/product_name(type, source_ref, "o");
        args.emplace_back(compile_flag);
        args.emplace_back(path_arg(source_ref));
        args.emplace_back(output_flag);
        args.emplace_back(path_arg(result.object));
        args.insert(args.end(), modules_args.begin(), modules_args.end());
        Process p = Process::spawn_noredir(_config.program_path,_config.working_directory, args);
        return p.waitpid_status();
    }

}
