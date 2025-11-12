#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include "../../utils/product_name.hpp"
#include <filesystem>

class CompilerClang : public AbstractCompiler {
public:

    virtual int compile(
        const OriginEnv &env,
        const std::filesystem::path &source, 
        ModuleType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const override;
    
    virtual int link(std::span<const std::filesystem::path> objects) const override;

        virtual std::string preproces(
                const OriginEnv &env,
                const std::filesystem::path &file) const override;

    CompilerClang(Config config):_config(config) {}


    static std::unique_ptr<AbstractCompiler> create(
        std::span<const ArgumentString> arguments,
        std::filesystem::path workdir
    );

    virtual bool generate_compile_command(const OriginEnv &env,
                                        const std::filesystem::path &source, 
                                        ModuleType type,
                                        std::span<const ModuleMapping> modules,
                                        std::vector<ArgumentString> &result) const override;


    virtual void initialize_module_map(std::span<const ModuleMapping> ) override {}


    static constexpr auto preprocess_flag = ArgumentConstant("-E");
    static constexpr auto bmi_ext = ArgumentConstant("pcm");
    static constexpr auto fmodule_header_user = ArgumentConstant("-fmodule-header=user");
    static constexpr auto fmodule_header_system = ArgumentConstant("-fmodule-header=system");    
    static constexpr auto xcpp_system_header = ArgumentConstant("-xc++-system-header");
    static constexpr auto xcpp_header = ArgumentConstant("-xc++-header");
    static constexpr auto xcpp_module = ArgumentConstant("-xc++-module");
    static constexpr auto output_flag = ArgumentConstant("-o");
    static constexpr auto precompile_flag = ArgumentConstant("--precompile");
    static constexpr auto compile_flag = ArgumentConstant("-c");
    static constexpr auto fmodule_file = ArgumentConstant("-fmodule-file=");

protected:
    Config _config;
    
    static std::vector<ArgumentString> prepare_args(const OriginEnv &env) ;
  

    std::filesystem::path get_bmi_path(ModuleType type, std::filesystem::path source) const {
        return _config.working_directory/"bmi"/product_name(type, source, "pcm");
    };
    std::filesystem::path get_obj_path(ModuleType type, const std::filesystem::path source) const {
        return _config.working_directory/"obj"/product_name(type, source, "o");
    }
    
};