module;
#ifdef _WIN32
#define DNOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ShlObj.h>
#undef interface
#endif

#pragma comment(lib, "Shell32.lib")

module cairn.compiler.msvc;

import cairn.abstract_compiler;
import cairn.preprocess;
import cairn.utils.log;
import cairn.utils.utf8;
import cairn.utils.serializer;
import cairn.utils.serializer.rules;
import cairn.utils.process;
import cairn.origin_env;
import cairn.source_def;
import cairn.source_scanner;
import cairn.compile_commands;
import cairn.module_type;
import cairn.utils.env;
import cairn.utils.arguments;

import <fstream>;
import <iostream>;
import <string>;
import <filesystem>;
import <numeric>;
import <vector>;
import <map>;
import <unordered_map>;

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

    virtual std::string preproc_for_test(const std::filesystem::path &file) const override;

    virtual bool transitive_headers() const override {return true;}

protected:

    Config _config;
    EnvironmentCache _env_cache;
    std::filesystem::path _module_cache_path;
    std::filesystem::path _object_cache_path;
    std::filesystem::path _env_cache_path;
    StupidPreprocessor _preproc;

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
    void initialize_preproc();

    std::string run_preproc(std::span<const ArgumentString> args, std::filesystem::path workdir, std::filesystem::path file) const;

};


static constexpr auto preproc_D = ArgumentConstant("/D");
static constexpr auto preproc_U = ArgumentConstant("/U");
static constexpr auto preproc_u = ArgumentConstant("/u");
static constexpr auto preproc_I = ArgumentConstant("/I");
static constexpr auto preproc_1 = ArgumentConstant("1");



#ifdef _WIN32
static std::filesystem::path findVsWhere()
{
    wchar_t programFilesX86[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, programFilesX86))) return {};

    std::filesystem::path p = programFilesX86;
    p /= L"Microsoft Visual Studio/Installer/vswhere.exe";

    return p;
}
#else 
static std::filesystem::path findVsWhere() {
    auto env = SystemEnvironment::current();
    return AbstractCompiler::find_in_path("vswhere.exe",env);
}
#endif


CompilerMSVC::VariantSpec CompilerMSVC::parse_variant_spec(std::filesystem::path compiler_path) {
    auto fname = compiler_path.filename().string();
    auto fname_sv = std::string_view(fname);
    //example: cl.exe@x64-14.29.30133
    auto atpos = fname_sv.find('@');
    if (atpos == std::string_view::npos) {        
        return {"",""};
    }
    std::string_view variant = fname_sv.substr(atpos + 1);
    auto dashpos = variant.find('-');
    std::string_view arch;
    std::string_view ver;
    if (dashpos == std::string_view::npos) {
        arch = variant;
        return {std::string(arch), ""};
    }
    arch = variant.substr(0, dashpos);
    ver = variant.substr(dashpos + 1);
    return {std::string(arch), std::string(ver)};
}

CompilerMSVC::CompilerMSVC(Config config): _config(std::move(config)) {
    _module_cache_path = _config.working_directory / "ifc";
    _object_cache_path = _config.working_directory / "obj";
    _env_cache_path = _config.working_directory / "env_cache.bin";

    std::filesystem::create_directories(_config.working_directory);

    VariantSpec spec = parse_variant_spec(_config.program_path);
    bool loaded = load_environment_from_cache();
    if (loaded) {
        if (!(spec == _env_cache.variant)) {
            loaded = false;
        }
    }
    if (!loaded) {        
        if (spec.architecture.empty()) {
            Log::debug("MSVC: Using current environment");
            _env_cache.env = SystemEnvironment::current();
            _env_cache.variant = spec;
        } else {
            auto install_path = get_install_path(spec.compiler_version);
            _env_cache.env = capture_environment(install_path.string(), spec.architecture);
            Log::verbose("MSVC: Configured for: {}, version: {}",  spec.architecture, [&]{
                return std::filesystem::path(_env_cache.env["VSCMD_VER"]).string();});            
            _env_cache.variant = spec;
            save_environment_to_cache();
        }
    }

/*    if (Log::is_level_enabled(Log::Level::debug)) {
        std::ofstream f(_config.working_directory/"env.txt", std::ios::out|std::ios::trunc);
        SystemEnvironment::Buffer b;
        auto iter = _env_cache.env.posix_format(b);
        while (*iter) {
            std::string tmp;
            auto *c = *iter;
            while (*c) {tmp.push_back(static_cast<char>(*c));++c;}
            f << tmp << std::endl;
            ++iter;
        }
    }
*/
    auto n = _config.program_path.filename().wstring();
    auto s = n.rfind(L'@');
    if (s != n.npos) {
        _config.program_path = _config.program_path.parent_path()/n.substr(0,s);
    }
    _config.program_path = find_in_path(_config.program_path, _env_cache.env);

    std::filesystem::path detect_macros = _config.working_directory/"defines.txt";
    initialize_preproc();

}

void CompilerMSVC::prepare_for_build()
{
    std::filesystem::create_directories(_module_cache_path);
    std::filesystem::create_directories(_object_cache_path);
}

int CompilerMSVC::compile(const OriginEnv &env, const SourceDef &src, std::span<const SourceDef> modules, CompileResult &result) const
{
    auto args = build_arguments( env, src, modules, result);
    if (!args.empty()) {
        int r = invoke( env.working_dir, args);
        if (r) {
            dump_failed_cmdline(_config, env.working_dir, args);;
            return r;   
        }
    }
    return 0;
}

int CompilerMSVC::link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const
{
    auto rsp = _object_cache_path/target.filename();
    rsp.replace_extension(".rsp");
    std::ofstream rspf(rsp, std::ios::trunc|std::ios::out);
    if (!rspf) {
        Log::error("Unable to create response file: {}", rsp.string());
        return 1;
    }
    rspf << "\xEF\xBB\xBF";
    for (const auto &s: objects) {
         Log::debug("Link object {}", [&]{
            return s.string();
         });
         rspf << '"' << s.u8string() << "\"\n";         
    }
    
    rspf.close();

    std::vector<ArgumentString> args;
    append_arguments(args, {"/nologo","/Fe{}"}, {path_arg(target)});
    args.insert(args.end(),_config.link_options.begin(), _config.link_options.end());
    append_arguments(args, {"@{}"}, {path_arg(rsp)});

    int r =  invoke( _config.working_directory, args);
    if (r) {
        dump_failed_cmdline(_config, _config.working_directory, args);
    }
    return r;
}
void CompilerMSVC::update_link_command(CompileCommandsTable &cc,  
        std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const {
        std::vector<ArgumentString> args = _config.link_options;
        append_arguments(args, {"/nologo","/Fe{}"}, {path_arg(output)});
        for (const auto &x: objects) args.push_back(path_arg(x));
        cc.update(cc.record(_config.working_directory, {}, _config.program_path, std::move(args), output));
    }


SourceScanner::Info CompilerMSVC::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    auto args = prepare_args(env,_config,'/');
    auto out = run_preproc(args, env.working_dir, file);

    auto info =  SourceScanner::scan_string(out);
    std::array<std::vector<SourceScanner::Reference> *,2> to_process({&info.required, &info.exported});
    for (auto &r: to_process) {
        for (auto &s: *r) {
            if (s.type == ModuleType::user_header) {
                s.name = (env.working_dir/s.name).lexically_normal().string();
            }
        }
    }
    return info;
}

void CompilerMSVC::update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const  {

    CompileResult res;
    auto args = build_arguments( env, src, modules, res);    
    auto out = res.interface.empty()?std::move(res.object):std::move(res.interface);
    cc.update(cc.record(env.working_dir, src.path, _config.program_path, std::move(args), std::move(out)));
}


bool CompilerMSVC::initialize_build_system(BuildSystemConfig)
{
    return false;
}

bool CompilerMSVC::commit_build_system()
{
    return false;
}

bool CompilerMSVC::load_environment_from_cache()
{
    try {
        std::ifstream file(_env_cache_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        deserialize_from_stream(file, _env_cache);
    } catch (std::exception &e) {
        Log::warning("failed to deserialize, dropping cache: {}", e.what());
    }
    return true;
}

void CompilerMSVC::save_environment_to_cache() {
    std::ofstream file(_env_cache_path,  std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open MSVC environment cache for writing");
    }
    serialize_to_stream(file, _env_cache);
}

std::filesystem::path CompilerMSVC::get_install_path(std::string_view version_spec)
{
    auto vswhere_path = findVsWhere();
    if (vswhere_path.empty() || !std::filesystem::exists(vswhere_path)) {
        throw std::runtime_error("vswhere.exe not found, cannot locate Visual Studio installation");
    }
    std::vector<ArgumentString> args;
    if (!version_spec.empty()) {
        append_arguments(args, {"-prerelease", "-version", "{}"}, {string_arg(version_spec)});
    } else {
        append_arguments(args, {"-latest"}, {});
    }
    append_arguments(args, {"-products","*","-property","InstallationPath"},{});
    auto proc = Process::spawn(vswhere_path, std::filesystem::current_path(), args, Process::output);
    std::string output;
    {
        std::istream &outstream = *proc.stdout_stream;
        std::getline(outstream, output);
        while (!output.empty() && std::isspace(output.back()) ) output.pop_back();
        if (output.empty()) {
            Log::error("Specified version is not installed. These versions are installed:");
            args.clear();
            append_arguments(args, {"-prerelease","-property","installationVersion"},{});
            Process p = Process::spawn(vswhere_path, std::filesystem::current_path(), args, Process::no_streams);
            p.waitpid_status();
            throw std::runtime_error("Failed to initialize the compiler");
        }
    }
    int status = proc.waitpid_status();
    if (status != 0) {
        throw std::runtime_error("vswhere.exe failed to locate Visual Studio installation");
    }
    return output;
}

SystemEnvironment CompilerMSVC::capture_environment(std::string_view install_path, std::string_view arch)
{
    std::filesystem::path vcvarsall = std::filesystem::path(install_path) / "VC"/"Auxiliary"/"Build"/"vcvarsall.bat";
    if (!std::filesystem::exists(vcvarsall)) {
        throw std::runtime_error("vcvarsall.bat not found in Visual Studio installation");
    }

//    auto callArg = std::format(L"call \"{}\" {} && set", vcvarsall.wstring(), string_arg(arch));

    std::vector<ArgumentString> args;
    append_arguments(args, {"/k", "call","{}","{}"}, {path_arg(vcvarsall), string_arg(arch)});

    auto proc = Process::spawn("cmd.exe", std::filesystem::current_path(), args, Process::input_output);
    (*proc.stdin_stream) << "set" << std::endl;
    proc.stdin_stream.reset();
    proc.child_stdin_buf.reset();
    SystemEnvironment env;
    {
        std::string env_data {std::istreambuf_iterator<char>(*proc.stdout_stream), std::istreambuf_iterator<char>()};
        env = SystemEnvironment::parse(env_data);
    }
    int status = proc.waitpid_status();
    if (status != 0) {
        throw std::runtime_error("Failed to capture MSVC environment");
    }
    return env;
}

std::vector<ArgumentString> CompilerMSVC::build_arguments(const OriginEnv &env, const SourceDef &src, std::span<const SourceDef> modules, CompileResult &result) const
{
    auto args = prepare_args(env,_config,'/');
    auto pdb = _object_cache_path/intermediate_file(src, ".pdb");
    append_arguments(args,{"/nologo","/ifcSearchDir","{}","/Fd{}","/c"}, {path_arg(_module_cache_path), path_arg(pdb)});
    for (const auto &r : modules) {
        switch (r.type) {
            case ModuleType::system_header:
                append_arguments(args, {"/headerUnit:angle","{}={}"},{string_arg(r.name), path_arg(r.path)});
                break;
            case ModuleType::user_header:
                append_arguments(args, {"/headerUnit","{}={}"},{string_arg(r.name), path_arg(r.path)});
                break;
            default:
                break;
        }
    }
    switch (src.type) {
        case ModuleType::system_header:
            result.interface = _module_cache_path/intermediate_file(src, ".ifc");
            append_arguments(args,{"/exportHeader","/headerName:angle","/ifcOutput","{}","{}"},
                            {path_arg(result.interface),path_arg(src.path)});
            return args;
        case ModuleType::user_header:
            result.interface = _module_cache_path/intermediate_file(src, ".ifc");
            append_arguments(args,{"/exportHeader","/headerName:quote","/ifcOutput","{}","{}"},
                            {path_arg(result.interface), path_arg(src.path)});
            return args;
        case ModuleType::partition:
            result.interface = _module_cache_path/map_module_name(src.name);
            result.object = _object_cache_path/intermediate_file(src, ".obj");
            append_arguments(args,{"/internalPartition","/ifcOutput","{}","/Fo{}","{}"},
                            {path_arg(result.interface), path_arg(result.object),path_arg(src.path)});
            return args;
        case ModuleType::interface:
            result.interface = _module_cache_path/map_module_name(src.name);
            result.object = _object_cache_path/intermediate_file(src, ".obj");            
            append_arguments(args,{"/interface","/ifcOutput","{}","/Fo{}","{}"},
                            {path_arg(result.interface), path_arg(result.object),path_arg(src.path)});
            return args;
        case ModuleType::implementation:
        case ModuleType::source:
            result.object = _object_cache_path/intermediate_file(src, ".obj");            
            append_arguments(args,{"/Fo{}","{}"},{ path_arg(result.object),path_arg(src.path)});
            return args;
        default:
            throw std::runtime_error("Reached unreachable code");
    }
}


std::string CompilerMSVC::map_module_name(const std::string_view &name) {
    std::string out;
    out.resize(name.size()+4,0);
    auto iter = std::transform(name.begin(), name.end(), out.begin(), [](char c) -> char{
        return c==':'? c = '-': c;
    });
    std::string_view ext(".ifc");
    std::copy(ext.begin(),ext.end(),iter);
    return out;

}



int CompilerMSVC::invoke(const std::filesystem::path &workdir, 
    std::span<const ArgumentString> arguments) const
{
    if (_disable_build) return 0;

    Process p = Process::spawn(_config.program_path, workdir, arguments, Process::output, _env_cache.env);
    std::string dummy(std::istreambuf_iterator<char>(*p.stdout_stream), std::istreambuf_iterator<char>());
    int r =  p.waitpid_status();
    int lines =std::accumulate(dummy.begin(), dummy.end(), 0, [](int a, char c){return a+(c == '\n'?1:0);});
    if (lines > 1) {    //attempt to remove filename from output
        Log::verbose("{}", dummy);   //any larger output is displayed
    }
    return r;

}

CompilerMSVC::SourceStatus CompilerMSVC::source_status(ModuleType t, const std::filesystem::path &file, std::filesystem::file_time_type tm) const
{
    //in case of clang we cannot detect a change in system header
    if (t == ModuleType::system_header) return SourceStatus::not_modified;
    return AbstractCompiler::source_status(t,file,tm);
}

std::unique_ptr<AbstractCompiler> create_compiler_msvc( AbstractCompiler::Config config) {
    return std::make_unique<CompilerMSVC>(std::move(config));
}

constexpr std::string_view all_compilers_macros[] = {
    "__cplusplus",
    "__DATE__",
    "__FILE__",
    "__LINE__",
    "__STDC__",
    "__STDC_HOSTED__",
    "__STDC_NO_ATOMICS__",
    "__STDC_NO_COMPLEX__",
    "__STDC_NO_THREADS__",
    "__STDC_NO_VLA__",
    "__STDC_VERSION__",
    "__STDCPP_DEFAULT_NEW_ALIGNMENT__",
    "__STDCPP_THREADS__",
    "__TIME__",
    "__ARM_ARCH",
    "__ATOM__",
    "__AVX__",
    "__AVX2__",
    "__AVX512BW__",
    "__AVX512CD__",
    "__AVX512DQ__",
    "__AVX512F__",
    "__AVX512VL__",
    "__AVX10_VER__",
    "_CHAR_UNSIGNED",
    "__CLR_VER",
    "_CONTROL_FLOW_GUARD",
    "__COUNTER__",
    "__cplusplus_winrt",
    "_CPPRTTI",
    "_CPPUNWIND",
    "_DEBUG",
    "_DLL",
    "_INTEGRAL_MAX_BITS",
    "__INTELLISENSE__",
    "_ISO_VOLATILE",
    "_KERNEL_MODE",
    "_M_AMD64",
    "_M_ARM",
    "_M_ARM_ARMV7VE",
    "_M_ARM_FP",
    "_M_ARM64",
    "_M_ARM64EC",
    "_M_CEE",
    "_M_CEE_PURE",
    "_M_CEE_SAFE",
    "_M_FP_CONTRACT",
    "_M_FP_EXCEPT",
    "_M_FP_FAST",
    "_M_FP_PRECISE",
    "_M_FP_STRICT",
    "_M_IX86",
    "_M_IX86_FP",
    "_M_X64",
    "_MANAGED",
    "_MSC_BUILD",
    "_MSC_EXTENSIONS",
    "_MSC_FULL_VER",
    "_MSC_VER",
    "_MSVC_LANG",
    "__MSVC_RUNTIME_CHECKS",
    "_MSVC_TRADITIONAL",
    "_MT",
    "_NATIVE_WCHAR_T_DEFINED",
    "_OPENMP",
    "_PREFAST_",
    "__SANITIZE_ADDRESS__",
    "__TIMESTAMP__",
    "_VC_NODEFAULTLIB",
    "_WCHAR_T_DEFINED",
    "_WIN32",
    "_WIN64",
    "_WINRT_DLL",
    "__cpp_aggregate_bases",
    "__cpp_aggregate_nsdmi",
    "__cpp_aggregate_paren_init",
    "__cpp_alias_templates",
    "__cpp_aligned_new",
    "__cpp_attributes",
    "__cpp_auto_cast",
    "__cpp_binary_literals",
    "__cpp_capture_star_this",
    "__cpp_char8_t",
    "__cpp_concepts",
    "__cpp_conditional_explicit",
    "__cpp_consteval",
    "__cpp_constexpr",
    "__cpp_constexpr_dynamic_alloc",
    "__cpp_constexpr_exceptions",
    "__cpp_constexpr_in_decltype",
    "__cpp_constinit",
    "__cpp_contracts",
    "__cpp_decltype",
    "__cpp_decltype_auto",
    "__cpp_deduction_guides",
    "__cpp_delegating_constructors",
    "__cpp_deleted_function",
    "__cpp_designated_initializers",
    "__cpp_enumerator_attributes",
    "__cpp_explicit_this_parameter",
    "__cpp_fold_expressions",
    "__cpp_generic_lambdas",
    "__cpp_guaranteed_copy_elision",
    "__cpp_hex_float",
    "__cpp_if_consteval",
    "__cpp_if_constexpr",
    "__cpp_impl_coroutine",
    "__cpp_impl_destroying_delete",
    "__cpp_impl_three_way_comparison",
    "__cpp_implicit_move",
    "__cpp_inheriting_constructors",
    "__cpp_init_captures",
    "__cpp_initializer_lists",
    "__cpp_inline_variables",
    "__cpp_lambdas",
    "__cpp_modules",
    "__cpp_multidimensional_subscript",
    "__cpp_named_character_escapes",
    "__cpp_namespace_attributes",
    "__cpp_noexcept_function_type",
    "__cpp_nontype_template_args",
    "__cpp_nontype_template_parameter_auto",
    "__cpp_nsdmi",
    "__cpp_pack_indexing",
    "__cpp_placeholder_variables",
    "__cpp_pp_embed",
    "__cpp_range_based_for",
    "__cpp_raw_strings",
    "__cpp_ref_qualifiers",
    "__cpp_return_type_deduction",
    "__cpp_rvalue_references",
    "__cpp_size_t_suffix",
    "__cpp_sized_deallocation",
    "__cpp_static_assert",
    "__cpp_static_call_operator",
    "__cpp_structured_bindings",
    "__cpp_template_parameters",
    "__cpp_template_template_args",
    "__cpp_threadsafe_static_init",
    "__cpp_trivial_relocatability",
    "__cpp_trivial_union",
    "__cpp_unicode_characters",
    "__cpp_unicode_literals",
    "__cpp_user_defined_literals",
    "__cpp_using_enum",
    "__cpp_variable_templates",
    "__cpp_variadic_friend",
    "__cpp_variadic_templates",
    "__cpp_variadic_using"
 };

void CompilerMSVC::create_macro_summary_file(const std::filesystem::path &target) {
    if (std::filesystem::exists(target)) return;
    std::ofstream out(target, std::ios::out|std::ios::trunc);
    for (auto &x: all_compilers_macros) {
        out << "#ifdef " << x << "\n" << x << "_m:" << x << "\n#endif\n";
    }
}

void CompilerMSVC::initialize_preproc() {
    auto sumfile = _config.working_directory/"macros.txt";
    create_macro_summary_file(sumfile);
    auto args = _config.compile_options;
    append_arguments(args, {"/nologo", "/EP", "/TP", "{}"}, {path_arg(sumfile)});
    auto proc = Process::spawn(_config.program_path, _config.working_directory, args, Process::output_error, _env_cache.env);
    auto &input = *proc.stdout_stream;
    std::string ln;
    while (!input.eof()) {
        std::getline(input, ln);
        while (!ln.empty() && std::isspace(ln.back())) ln.pop_back();
        if (ln.empty()) continue;
        auto sep = ln.find("_m:");
        if (sep == ln.npos) continue;
        auto name = ln.substr(0, sep);
        auto value = ln.substr(sep+3);
        _preproc.define_symbol(name, value);
    }

    auto r = _env_cache.env["INCLUDE"];
    std::vector<std::filesystem::path> includes;
    std::basic_string_view lst(r);
    while (!lst.empty()) {
        auto sep = lst.find(';');
        if (sep == lst.npos) {
            includes.push_back(std::filesystem::path(lst));
            lst = {};
        } else {
            includes.push_back(std::filesystem::path(lst.substr(0,sep)));
            lst = lst.substr(sep+1);
        }
    }
    _preproc.append_includes(includes);
}

std::string CompilerMSVC::run_preproc(std::span<const ArgumentString> args, std::filesystem::path workdir, std::filesystem::path file) const {
    auto preproc = _preproc;
    int stg = 0;
    for (ArgumentStringView x: args) {
        if (x == preproc_D) {stg = 'd';continue;}
        if (x == preproc_I) {stg = 'i';continue;}
        if (x == preproc_U) {stg = 'u';continue;}
        if (x == preproc_u) {stg = 'u';continue;}
        if (stg == 0) {
            if (x.substr(0,preproc_D.size()) == preproc_D) {stg = 'd'; x = x.substr(0,preproc_D.size());}
            if (x.substr(0,preproc_I.size()) == preproc_I) {stg = 'i'; x = x.substr(0,preproc_I.size());}
            if (x.substr(0,preproc_U.size()) == preproc_U) {stg = 'u'; x = x.substr(0,preproc_U.size());}
            if (x.substr(0,preproc_u.size()) == preproc_u) {stg = 'u'; x = x.substr(0,preproc_u.size());}
        }
        switch (stg) {
            case 'd':  {//define
                ArgumentStringView key;
                ArgumentStringView value;
                auto sep = x.find('=');
                if (sep == x.npos) {
                    key = x;
                    value = preproc_1;
                } else {
                    key = x.substr(0,sep);
                    value = x.substr(sep+1);
                }
                std::string key_u;
                std::string value_u;
                to_utf8(key.begin(), key.end(), std::back_inserter(key_u));
                to_utf8(value.begin(), value.end(), std::back_inserter(value_u));
                preproc.define_symbol(key_u, value_u);
                break;
            }
            case 'i': {//include
                std::filesystem::path p = (workdir/x).lexically_normal();
                preproc.append_includes(std::move(p));
                break;
            }
            case 'u': {//undef
                std::string key_u;
                to_utf8(x.begin(), x.end(), std::back_inserter(key_u));
                preproc.undef_symbol(key_u);
                break;
            }
        }
    }
    return preproc.run(workdir, file);

}

std::string CompilerMSVC::preproc_for_test(const std::filesystem::path &file) const {
    return run_preproc(_config.compile_options, std::filesystem::current_path(), file);
}

