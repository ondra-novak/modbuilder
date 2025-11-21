#include "compiler_msvc.hpp"
#include "../../utils/log.hpp"
#include "../../utils/utf_8.hpp"
#include "factory.hpp"
#include "../../utils/process.hpp"
#include "../../utils/serializer.hpp"
#include "../../utils/serialization_rules.hpp"
#include <future>

#include <stdexcept>
#include <fstream>
#include <ShlObj.h>
#undef interface

static constexpr auto preproc_D = ArgumentConstant("/D");
static constexpr auto preproc_U = ArgumentConstant("/U");
static constexpr auto preproc_u = ArgumentConstant("/u");
static constexpr auto preproc_I = ArgumentConstant("/I");

static constexpr auto all_preproc = std::array<ArgumentStringView, 4>({
    preproc_D, preproc_I, preproc_U, preproc_u
});


static std::filesystem::path findVsWhere()
{
    wchar_t programFilesX86[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, programFilesX86))) return {};

    std::filesystem::path p = programFilesX86;
    p /= L"Microsoft Visual Studio/Installer/vswhere.exe";

    return p;
}

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
            Log::debug("MSVC: Configured for: {}, version: {}",  spec.architecture, [&]{
                return std::filesystem::path(_env_cache.env["VSCMD_VER"]).string();});            
            _env_cache.variant = spec;
            save_environment_to_cache();
        }
    }

    if (Log::is_level_enabled(Log::Level::debug)) {
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

    auto n = _config.program_path.filename().wstring();
    auto s = n.rfind(L'@');
    if (s != n.npos) {
        _config.program_path = _config.program_path.parent_path()/n.substr(0,s);
    }
    _config.program_path = find_in_path(_config.program_path, _env_cache.env);
    _helper.start(1);

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

SourceScanner::Info CompilerMSVC::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    auto args = prepare_args(env,_config,'/');

    args.erase(std::remove_if(args.begin(), args.end(), [&,skip = false](const ArgumentString &s) mutable {
        if (skip) {
            skip = false;
            return false;
        }
        auto f1 = std::find(all_preproc.begin(), all_preproc.end(), s);
        if (f1 != all_preproc.end()) {
            skip = true;
            return false;
        }
        auto f2 = std::find_if(all_preproc.begin(), all_preproc.end(), [&](const ArgumentStringView &w) {
            return s.starts_with(w);
        });
        return (f2 == all_preproc.end());
    }), args.end());

    append_arguments(args, {"/nologo", "/EP", "{}"}, {path_arg(file)});
    auto proc = Process::spawn(_config.program_path, env.working_dir, args, Process::output_error, _env_cache.env);

    std::promise<std::string> errprom;
    _helper.push([&]() noexcept {
        try {
            std::string err((std::istreambuf_iterator<char>(*proc.stderr_stream)),
                            std::istreambuf_iterator<char>());
            errprom.set_value(std::move(err));
        } catch (...) {
            errprom.set_exception(std::current_exception());
        }
    });


    std::string output(std::istreambuf_iterator<char>(*proc.stdout_stream), std::istreambuf_iterator<char>());
    int status = proc.waitpid_status();
    std::string err = errprom.get_future().get();
    if (status != 0) {        
        dump_failed_cmdline(_config, env.working_dir, args);
        std::cerr << err << "\n";
        return {};
    }
    auto info =  SourceScanner::scan_string(output);
    for (auto &s: info.required) {
        if (s.type == ModuleType::user_header) {
            s.name = (env.working_dir/s.name).lexically_normal().string();
        }
    }
    return info;
}

bool CompilerMSVC::generate_compile_command(const OriginEnv &env, const SourceDef &src, std::span<const SourceDef> modules, std::vector<ArgumentString> &result) const
{
    CompileResult dummy;
    result = build_arguments(env, src, modules, dummy);
    if (result.empty()) return false;
    result.insert(result.begin(), path_arg(_config.program_path));
    return true;

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
    append_arguments(args, {"/C", "call","{}","{}","&&","set"}, {vcvarsall.wstring(), string_arg(arch)});

    auto proc = Process::spawn("cmd.exe", std::filesystem::current_path(), args, Process::output);
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
    append_arguments(args,{"/nologo","/ifcSearchDir","{}","/c"}, {path_arg(_module_cache_path)});
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


std::unique_ptr<AbstractCompiler> create_compiler_msvc( AbstractCompiler::Config config) {
    return std::make_unique<CompilerMSVC>(std::move(config));
}

int CompilerMSVC::invoke(const std::filesystem::path &workdir, 
    std::span<const ArgumentString> arguments) const
{
    Process p = Process::spawn(_config.program_path, workdir, arguments, Process::output, _env_cache.env);
    std::string dummy(std::istreambuf_iterator<char>(*p.stdout_stream), std::istreambuf_iterator<char>());
    int r =  p.waitpid_status();
    if (r) {
        std::cerr << dummy << std::endl;
    }
    return r;

}

CompilerMSVC::SourceStatus CompilerMSVC::source_status(ModuleType t, const std::filesystem::path &file, std::filesystem::file_time_type tm) const
{
    //in case of clang we cannot detect a change in system header
    if (t == ModuleType::system_header) return SourceStatus::not_modified;
    return AbstractCompiler::source_status(t,file,tm);
}
