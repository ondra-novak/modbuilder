#include "compiler_gcc.hpp"
#include <memory>
#include <utils/arguments.hpp>
#include <utils/process.hpp>


int CompilerGcc::link([[maybe_unused]] std::filesystem::path binary,
                        [[maybe_unused]] std::span<const std::filesystem::path> objects) const {
    throw std::runtime_error("not implemented");
}

std::string CompilerGcc::preproces(const std::filesystem::path &file) const {

    std::vector<ArgumentString> args = _config.compile_options;
    args.emplace_back(preprocess_flag);
    args.emplace_back(path_arg(file));

    Process p = Process::spawn(_config.program_path, _config.working_directory, std::move(args));

    std::string out((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());

    p.waitpid_status();
    return out;
}

std::unique_ptr<AbstractCompiler> CompilerGcc::create(std::span<const ArgumentString> arguments) {
    return std::make_unique<CompilerGcc>(parse_commandline(arguments));
}

int CompilerGcc::compile(ArgumentString source_ref, 
        ModuleReferenceType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const {

}
