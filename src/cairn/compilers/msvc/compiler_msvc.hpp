#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include "../../utils/env.hpp"
#include "../../utils/thread_pool.hpp"
#include <vector>

class CompilerMSVC: public AbstractCompiler {
public:

    CompilerMSVC(Config config);
    virtual std::string_view get_compiler_name() const override {
        return "msvc";
    }
    virtual void prepare_for_build() override;
    virtual int compile(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const override;
    virtual int link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const override;
    virtual SourceScanner::Info scan(const OriginEnv &env, const std::filesystem::path &file) const override;
    virtual void update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const  override;
    virtual void update_link_command(CompileCommandsTable &cc,  
                std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const override;


    virtual bool initialize_build_system(BuildSystemConfig ) override;
    virtual bool commit_build_system() override;
    virtual void initialize_module_map(std::span<const ModuleMapping> ) override {}

    struct VariantSpec {
        std::string architecture;
        std::string compiler_version;
        bool operator==(const VariantSpec &other) const  = default;

        template<typename Me, typename Arch>
        static void serialize(Me &me, Arch &arch) {
            arch(me.architecture, me.compiler_version);
        }

    };

    virtual SourceStatus source_status(ModuleType t, const std::filesystem::path &file, 
        std::filesystem::file_time_type tm) const override;

    struct EnvironmentCache {
        VariantSpec variant;
        SystemEnvironment env;

        template<typename Me, typename Arch>
        static void serialize(Me &me, Arch &arch) {
            arch(me.variant, me.env);
        }
    };


protected:

    Config _config;
    EnvironmentCache _env_cache;
    std::filesystem::path _module_cache_path;
    std::filesystem::path _object_cache_path;
    std::filesystem::path _env_cache_path;

    mutable ThreadPool _helper;

    bool load_environment_from_cache();
    void save_environment_to_cache();
    static std::filesystem::path get_install_path(std::string_view version_spec);
    static SystemEnvironment capture_environment(std::string_view install_path, std::string_view arch);
  
    static VariantSpec parse_variant_spec(std::filesystem::path compiler_path);
    std::vector<ArgumentString> build_arguments(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const;


    static std::string map_module_name(const std::string_view &name);

    int invoke( 
        const std::filesystem::path &workdir, 
        std::span<const ArgumentString> arguments) const;

    void create_macro_summary_file(const std::filesystem::path &target);
};
