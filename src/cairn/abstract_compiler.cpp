#ifdef _MSC_VER
module;
#include <filesystem> //msvc requires
#endif 

export module cairn.abstract_compiler;

import cairn.module_type;
import cairn.source_def;
import cairn.utils.arguments;
import cairn.origin_env;
import cairn.source_scanner;
import cairn.utils.process;
import cairn.utils.log;
import cairn.utils.env;
import cairn.compile_commands;
import <string_view>;
import <format>;
import <filesystem>;
import <vector>;
import <span>;



export class AbstractCompiler {
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

    virtual std::string preproc_for_test(const std::filesystem::path &file) const = 0;

    ///compiler requires to include header transitive (MSVC)
    virtual bool transitive_headers() const {return false;}


protected:
    bool _disable_build = false;

};


int AbstractCompiler::invoke(const Config &cfg, 
    const std::filesystem::path &workdir, 
    std::span<const ArgumentString> arguments) const
{
    if (_disable_build) return 0;
    Process p = Process::spawn(cfg.program_path, workdir, arguments, Process::no_streams);
    return p.waitpid_status();

}

std::filesystem::path AbstractCompiler::intermediate_file(const SourceDef &src, std::string_view ext) {

    if (ext.starts_with('.')) ext = ext.substr(1);
    
    std::hash<std::filesystem::path> path_hasher;
    std::size_t h1 = path_hasher(src.path.parent_path());
    std::string whole_name = std::format("{}_{:x}.{}", src.path.stem().string(), h1, ext);
    return whole_name;

}


std::vector<ArgumentString> AbstractCompiler::prepare_args(const OriginEnv &env, const Config &config, char switch_char) {
    std::vector<ArgumentString> out;
    ArgumentString a;
    for (const auto &i: env.includes) {
        auto s = path_arg(i);
        a.clear();
        a.push_back(switch_char);
        a.push_back('I');
        a.append(s);
        out.push_back(a);
    }
    for (const auto &o: env.options) {
        out.push_back(string_arg(o));
    }
    for (const auto &c: config.compile_options) {
        out.push_back(c);
    }

    return out;        
}

void AbstractCompiler::dump_failed_cmdline(const Config &cfg, const std::filesystem::path &workdir, std::span<const ArgumentString> cmdline) {
    Log::error("Failed command: {}", [&]{
        std::ostringstream s;
        s << cfg.program_path.string();
        for (const auto &x: cmdline) {
            s << " " << std::filesystem::path(x);
        };
        return std::move(s).str();
    });
    Log::verbose("Working directory: {}", workdir.string());
}

std::filesystem::path AbstractCompiler::find_in_path(std::filesystem::path name, const SystemEnvironment &env)
{    
    auto lst_str =env["PATH"];
#ifdef _WIN32
    wchar_t sep = L';';
#else  
    wchar_t sep = ':';
#endif

    while (!lst_str.empty()) {
        auto n = lst_str.find(sep);
        auto p = n == lst_str.npos?lst_str:lst_str.substr(0,n);
        lst_str = n == lst_str.npos?decltype(lst_str)():lst_str.substr(n+1);
        std::filesystem::path dir(p);
        std::filesystem::path candidate = dir / name;
        if (std::filesystem::exists(candidate) &&
            std::filesystem::is_regular_file(candidate)) {
                return candidate;                
        }

    }

    if (!std::filesystem::is_regular_file(name)) {
        throw std::runtime_error("Unable to find executable: "+name.string());
    }
    return name;
};

