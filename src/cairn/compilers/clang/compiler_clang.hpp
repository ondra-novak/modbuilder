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
    
    virtual int link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const override;

    virtual SourceScanner::Info scan(const OriginEnv &env, const std::filesystem::path &file) const override;


    std::string preprocess(const OriginEnv &env, const std::filesystem::path &file) const;

    CompilerClang(Config config);

    virtual bool initialize_build_system(BuildSystemConfig ) override {
        return false;
    }

    virtual bool commit_build_system() override {
        return false;
    }


    virtual bool generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        std::vector<ArgumentString> &result) const override;


    virtual void initialize_module_map(std::span<const SourceDef> ) override {}


    static constexpr auto stdcpp=ArgumentConstant("-std=c++");

    //preprocessor options
    static constexpr auto preproc_D = ArgumentConstant("-D");
    static constexpr auto preproc_U = ArgumentConstant("-U");
    static constexpr auto preproc_I = ArgumentConstant("-I");
    static constexpr auto preproc_define_macro = ArgumentConstant("--define-macro");
    static constexpr auto preproc_undefine_macro = ArgumentConstant("--undefine-macro");
    static constexpr auto preproc_include_directory = ArgumentConstant("--include-directory");
    
    static constexpr auto all_preproc = std::array<ArgumentStringView, 6>({
        preproc_D, preproc_I, preproc_U, preproc_define_macro, preproc_undefine_macro, preproc_include_directory
    });


protected:
    Config _config;
    std::filesystem::path _module_cache;
    std::filesystem::path _object_cache;
    Version _version;
    
  

    std::filesystem::path get_bmi_path(const SourceDef &src) const {
        auto n = src.name;
        for (auto &c: n) if (c == ':') c = '-';
        std::filesystem::path fname(n);
        fname.replace_extension(".pcm");
        return _config.working_directory/"pcm"/fname;
    };

    std::filesystem::path get_obj_path(const SourceDef &src) const {
        return _config.working_directory/"obj"/intermediate_file(src,".o");
    }
    std::filesystem::path get_hdr_bmi_path(const SourceDef &src) const {
        return _config.working_directory/"pcm"/intermediate_file(src,"~hdr.pcm");
    }
    std::vector<ArgumentString> build_arguments(bool precompile_stage,  const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const;
    

    static Version get_clang_version(Config &cfg);
};