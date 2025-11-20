#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include "../../utils/version.hpp"
#include "../../utils/thread_pool.hpp"
#include "factory.hpp"
#include <filesystem>
#include <array>

class CompilerGcc : public AbstractCompiler {
public:

    virtual std::string_view get_compiler_name() const override {
        return "gcc";
    }

    virtual void prepare_for_build() override;


    virtual int compile(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const override;
    
    virtual int link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const override;

    virtual SourceScanner::Info scan(const OriginEnv &env, const std::filesystem::path &file) const override;


    std::pair<std::string,std::string> preprocess(const OriginEnv &env, const std::filesystem::path &file) const;

    CompilerGcc(Config config);

    virtual bool initialize_build_system(BuildSystemConfig ) override {return false;}

    virtual bool commit_build_system() override {return false;}


    virtual bool generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        std::vector<ArgumentString> &result) const override;


    virtual void initialize_module_map(std::span<const ModuleMapping> ) override;

    virtual SourceStatus source_status(ModuleType , const std::filesystem::path &file, std::filesystem::file_time_type tm) const override;

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
    std::filesystem::path _module_mapper;
    Version _version;
    mutable ThreadPool _helper;
    

    static Version get_gcc_version(Config &cfg);

    std::vector<ArgumentString> build_arguments(const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const;


};
