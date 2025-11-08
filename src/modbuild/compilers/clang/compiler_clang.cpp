#include "compiler_clang.hpp"
#include <memory>
#include <utils/arguments.hpp>
#include <utils/process.hpp>




int CompilerClang::compile([[maybe_unused]] std::filesystem::path source,
                            [[maybe_unused]] std::span<const ModuleMapping> modules,
                            [[maybe_unused]] CompileResult &result) const {
    throw std::runtime_error("not implemented");
}

int CompilerClang::link([[maybe_unused]] std::filesystem::path binary,
                            [[maybe_unused]] std::span<const std::filesystem::path> objects) const {
  throw std::runtime_error("not implemented");
}

std::string CompilerClang::preproces(const std::filesystem::path &file) const {

  std::vector<ArgumentString> args = _config.compile_options;
  args.emplace_back(preprocess_flag);
  args.emplace_back(path_arg(file));

  Process p = Process::spawn(_config.program_path, std::move(args));

 std::string out((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());

    p.waitpid_status();
    return out;
}

std::unique_ptr<AbstractCompiler> CompilerClang::create(std::span<const ArgumentString> arguments) {
    return std::make_unique<CompilerClang>(parse_commandline(arguments));
}