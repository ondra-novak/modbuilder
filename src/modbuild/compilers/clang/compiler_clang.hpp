#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include "../../utils/version.hpp"
#include <filesystem>

class CompilerClang : public AbstractCompiler {
public:

    virtual std::string_view get_compiler_name() const override {
        return "clang";
    }

    virtual int compile(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const override;
    
    virtual int link(std::span<const std::filesystem::path> objects) const override;

        virtual std::string preproces(
                const OriginEnv &env,
                const std::filesystem::path &file) const override;

    CompilerClang(Config config);



    virtual bool generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
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
    static constexpr auto version_flag = ArgumentConstant("--version");
    static constexpr auto fprebuild_module_path = ArgumentConstant("-fprebuilt-module-path=");

protected:
    Config _config;
    std::filesystem::path _module_cache;
    std::filesystem::path _object_cache;
    Version _version;
    
  

    std::filesystem::path get_bmi_path(const SourceDef &src) const {
        auto n = src.name;
        for (auto &c: n) if (c == ':') c = '_';
        std::filesystem::path fname(n);
        fname.replace_extension(".pcm");
        return _config.working_directory/"pcm"/fname;
    };

    std::filesystem::path get_obj_path(const SourceDef &src) const {
        return _config.working_directory/"obj"/intermediate_file(src,".o");
    }
    std::filesystem::path get_hdr_bmi_path(const SourceDef &src) const {
        return _config.working_directory/"pcm"/intermediate_file(src,".pcm");
    }
    std::vector<ArgumentString> build_arguments(bool precompile_stage,  const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const;
    
};