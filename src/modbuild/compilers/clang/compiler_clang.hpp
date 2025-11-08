#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include <filesystem>

class CompilerClang : public AbstractCompiler {
public:

    virtual int compile(const std::filesystem::path &source_ref, 
        ModuleReferenceType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const override;
    
    virtual int link(std::filesystem::path binary, 
        std::span<const std::filesystem::path> objects) const override;

    virtual std::string preproces(const std::filesystem::path &file) const override;

    CompilerClang(Config config):_config(config) {}


    static std::unique_ptr<AbstractCompiler> create(
        std::span<const ArgumentString> arguments,
        std::filesystem::path workdir
    );


    static constexpr auto preprocess_flag = ArgumentConstant("-E");
    static constexpr auto bmi_ext = ArgumentConstant("pcm");
    static constexpr auto fmodule_header_user = ArgumentConstant("-fmodule-header=user");
    static constexpr auto fmodule_header_system = ArgumentConstant("-fmodule-header=system");    
    static constexpr auto xcpp_system_header = ArgumentConstant("-xc++-system-header");
    static constexpr auto xcpp_header = ArgumentConstant("-xc++-header");
    static constexpr auto output_flag = ArgumentConstant("-o");
    static constexpr auto precompile_flag = ArgumentConstant("--precompile");
    static constexpr auto compile_flag = ArgumentConstant("-c");
    static constexpr auto fmodule_file = ArgumentConstant("-fmodule-file=");

protected:
    Config _config;

    


    
};