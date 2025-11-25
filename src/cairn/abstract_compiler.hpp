#pragma once

#include "module_type.hpp"
#include "origin_env.hpp"
#include "utils/arguments.hpp"
#include "utils/which.hpp"
#include "scanner.hpp"
#include "source_def.hpp"
#include <filesystem>
#include <vector>
#include <span>

class SystemEnvironment;
class CompileCommandsTable;

class AbstractCompiler {
public:
    

    struct CompileResult {
        std::filesystem::path interface;
        std::filesystem::path object;
    };

    struct ModuleMapping: SourceDef {
        std::filesystem::path work_dir;    //contains copy of origin of the module
    };

    struct Config {
        std::filesystem::path program_path;
        std::vector<ArgumentString> compile_options;
        std::vector<ArgumentString> link_options;
        std::filesystem::path working_directory;
    };

    struct BuildSystemConfig {
        unsigned int threads;
        bool keep_going;
    };

    virtual ~AbstractCompiler() = default;

    ///performs actions to prepare compiler for build (for example creates working directory)
    virtual void prepare_for_build() = 0;

    virtual std::string_view get_compiler_name() const = 0;

    ///Initialize module map
    /** Use this before build. Some compilers can benefit of this, other not
        @param module_interface_cpp_list list of modules and interfaces. The path to interface is path to original
        cpp(m) file, not path to compiled .bmi

        You must initialize map before you can compile any module. 
        This feature is used by gcc module-mapper. Gcc fails to compile module if it was not
        properly anounced. Clang and msvc ignores this feature.
    */
    virtual void initialize_module_map(std::span<const ModuleMapping> module_interface_cpp_list) = 0;


    ///Initializes compiler specific build system, (for example ninja, make, MSBuild, if configured)
    /**
     * @param config build system configuration
     * @retval true build system is ready, following commands will be executed by the build system. You
     * need to call commit_build_system() to tell the build system to commit all scheduled work
     * @retval false the build system is not ready or not supported, so the caller must orchestrate
     * the build process
     * 
     */
    virtual bool initialize_build_system(BuildSystemConfig config) = 0;

    ///Commit all actions scheduled after initialization
    /**
     * @retval true success build
     * @retval false failure
     */
    virtual bool commit_build_system() = 0;

    ///Compile source file
    /**
     * @param env environment of file's origin. Can contain additional options, such a working directory, etc
     * @param source path to compiled source
     * @param type type of module to compile
     * @param module list of module mapping - it contains module name and path to BMI file (as returned from previous compile)
     * @param result contains compile results
     * @return if 0 returned, compilation is success, otherwise, there were an error and compilation should stop here.
     * 
     * @note compiler emits results to stdout/stderr
     */
    virtual int compile(
        const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const = 0;
    


    ///Perform link operation
    /**
     * @param objects list of all objects
     * @param output output executable
     * @return linker status code, 0 = success
     */
    virtual int link(std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const = 0;


    ///Update compile commands table
    /**
     * @param cc reference to compile commands table
     * @param env source origin
     * @param src source definition
     * @param modules list of required modules (refering BMI files)
     */
    virtual void update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const = 0;
    virtual void update_link_command(CompileCommandsTable &cc,  
                std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const = 0;

    ///Perform scan operation
    /**
     * @param env source origin
     * @param file file to scan
     * @return Scanner's information structure. If the file cannot be scanned, it returns empty status. The compiles will handle errors later
     */
    virtual SourceScanner::Info scan(const OriginEnv &env, const std::filesystem::path &file) const = 0;


    enum class SourceStatus {
        not_modified,
        modified,
        not_exist
    };

    virtual SourceStatus source_status(ModuleType , const std::filesystem::path &file, std::filesystem::file_time_type tm) const {
        std::error_code ec;
        auto lwt = std::filesystem::last_write_time(file, ec);
        if (ec != std::error_code{}) return SourceStatus::not_exist;
        if (lwt > tm) return SourceStatus::modified;
        return SourceStatus::not_modified;


    }

    static constexpr auto compile_flag = ArgumentConstant("--compile:");
    static constexpr auto link_flag = ArgumentConstant("--link:");


    static inline void ensure_path_exists(const std::filesystem::path &file_path) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    int invoke(const Config &cfg, 
        const std::filesystem::path &workdir, 
        std::span<const ArgumentString> arguments) const;

    static std::vector<ArgumentString> prepare_args(const OriginEnv &env, const Config &config, char switch_char);


    static std::filesystem::path intermediate_file( const SourceDef &src, std::string_view ext);
    static void dump_failed_cmdline(const Config &cfg, const std::filesystem::path &workdir, std::span<const ArgumentString> cmdline);
    static std::filesystem::path find_in_path(std::filesystem::path name, const SystemEnvironment &env);

    void dry_run(bool enabled) {
        _disable_build = enabled;
    }

protected:
    bool _disable_build = false;
};
