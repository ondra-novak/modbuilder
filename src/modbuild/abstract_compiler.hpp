#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <span>
#include "module_type.hpp"
#include "origin_env.hpp"
#include "utils/arguments.hpp"
#include "utils/which.hpp"

class AbstractCompiler {
public:

    struct CompileResult {
        std::filesystem::path interface;
        std::filesystem::path object;
        std::vector<ArgumentString> compile_arguments;
    };

    struct ModuleMapping {
        std::string logical_name;
        std::filesystem::path interface;
    };

    struct Config {
        std::filesystem::path program_path;
        std::vector<ArgumentString> compile_options;
        std::vector<ArgumentString> link_options;
        std::filesystem::path working_directory;
        bool dry_run;
    };

    virtual ~AbstractCompiler() = default;

    ///Initialize module map
    /** Use this before build. Some compilers can benefit of this, other not
        @param module_interface_cpp_list list of modules and interfaces. The path to interface is path to original
        cpp(m) file, not path to compiled .bmi

        You must initialize map before you can compile any module. 
        This feature is used by gcc module-mapper. Gcc fails to compile module if it was not
        properly anounced. Clang and msvc ignores this feature.
    */
    virtual void initialize_module_map(std::span<const ModuleMapping> module_interface_cpp_list) = 0;

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
        const std::filesystem::path &source, 
        ModuleType type,
        std::span<const ModuleMapping> modules,
        CompileResult &result) const = 0;
    

    ///Generates compile command
    /**
     * @param env environment of file's origin
     * @param source source file
     * @param type type of module
     * @param modules list of module mappings - it contains module name and path to BMI file (as returned from previous compile)
     * @param result filled with command which was execute to compile
     * @retval true generated
     * @retval false this file cannot be added to compiled_commands
     */
    virtual bool generate_compile_command(const OriginEnv &env,
                                          const std::filesystem::path &source, 
                                          ModuleType type,
                                          std::span<const ModuleMapping> modules,
                                          std::vector<ArgumentString> &result) const = 0;

    virtual int link(std::span<const std::filesystem::path> objects) const = 0;


    
    ///Preproces source file
    /**
     * @param env environment of file's origin. Can contain additional options, such a working directory, etc
     * @param file file to preprocess
     * @return preprocessed file as string. If the file cannot be preprocessed, it returns empty string
     */
    virtual std::string preproces(
        const OriginEnv &env,
        const std::filesystem::path &file) const = 0;
    

    static constexpr auto compile_flag = ArgumentConstant("--compile:");
    static constexpr auto link_flag = ArgumentConstant("--link:");

    enum class ParamKind {
        common, compile, link
    };

    static Config parse_commandline(const std::span<const ArgumentString> &args, std::filesystem::path working_dir) {
        Config out;
        ParamKind st = ParamKind::common;
        if (args.empty()) return out;

        auto found = find_in_path(args[0]);
        if (found.has_value()) out.program_path = std::move(found.value());
        else out.program_path = args[0];

        out.working_directory = std::move(working_dir);

        auto params = args.subspan(1);

        for (auto &a: params) {
            if (a == compile_flag) st = ParamKind::compile;
            else if (a == link_flag) st = ParamKind::link;
            else { 
                switch (st) {
                    case ParamKind::common: out.compile_options.push_back(a);
                                        out.link_options.push_back(a);break;
                    case ParamKind::compile: out.compile_options.push_back(a);break;
                    case ParamKind::link: out.link_options.push_back(a);break;
                }
            }            
        }
        return out;
    }

    static inline void ensure_path_exists(const std::filesystem::path &file_path) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    static int spawn_compiler(const Config &cfg, const std::filesystem::path &workdir, std::span<const ArgumentString> arguments, std::vector<ArgumentString> *dump_cmdline = nullptr);
    
};
