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

    struct SourceDef {
        ModuleType type;    //type of compiled module
        std::string name;   //name of compiled module 
        std::filesystem::path path;  //path to source file / interface file
    };

    struct CompileResult {
        std::filesystem::path interface;
        std::filesystem::path object;
    };

    struct ModuleMapping {
        ModuleType type;    //type of module (we need this to know whether it is header or standard module)
        std::string logical_name;   //logical module name
        std::filesystem::path interface;    //path to interface (bmi, pcm, gcm, etc)
    };

    struct Config {
        std::filesystem::path program_path;
        std::vector<ArgumentString> compile_options;
        std::vector<ArgumentString> link_options;
        std::filesystem::path working_directory;
    };

    virtual ~AbstractCompiler() = default;


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
    virtual bool generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
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


    static inline void ensure_path_exists(const std::filesystem::path &file_path) {
        std::filesystem::create_directories(file_path.parent_path());
    }

    static int invoke(const Config &cfg, 
        const std::filesystem::path &workdir, 
        std::span<const ArgumentString> arguments);

    static std::vector<ArgumentString> prepare_args(const OriginEnv &env);

    static std::filesystem::path intermediate_file( const SourceDef &src, std::string_view ext);
};
