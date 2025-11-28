module cairn.compiler.clang;

import cairn.compile_commands;
import cairn.utils.log;
import cairn.utils.utf8;
import cairn.module_type;
import cairn.utils.arguments;
import cairn.utils.process;
import cairn.abstract_compiler;
import cairn.utils.version;
import cairn.origin_env;
import cairn.source_def;
import cairn.source_scanner;
import cairn.utils.env;
import cairn.utils.log;
import cairn.utils.threadpool;
import cairn.preprocess;
import cairn.gnu_compiler_setup;

import <algorithm>;
import <filesystem>;
import <iterator>;
import <fstream>;
import <regex>;
import <stdexcept>;
import <memory>;
import <array>;
import <span>;
import <optional>;
import <unordered_map>;

class CompilerClang : public AbstractCompiler {
public:

    virtual std::string_view get_compiler_name() const override {
        return "clang";
    }

    virtual void prepare_for_build() override;


    virtual int compile(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        CompileResult &result) const override;
    
    virtual int link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const override;

    virtual SourceScanner::Info scan(const OriginEnv &env, const std::filesystem::path &file) const override;

    CompilerClang(Config config);

    virtual bool initialize_build_system(BuildSystemConfig ) override {
        return false;
    }

    virtual bool commit_build_system() override {
        return false;
    }


    virtual void update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const  override;


    virtual void initialize_module_map(std::span<const ModuleMapping> ) override {}


    static constexpr auto stdcpp=ArgumentConstant("-std=c++");

     virtual void update_link_command(CompileCommandsTable &cc,  
                std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const override; 
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


    virtual SourceStatus source_status(ModuleType , const std::filesystem::path &file, std::filesystem::file_time_type tm) const override;

    virtual std::string preproc_for_test(const std::filesystem::path &file) const override;

protected:
    Config _config;
    std::filesystem::path _module_cache;
    std::filesystem::path _object_cache;
    Version _version;
    StupidPreprocessor _preproc;
  

    std::filesystem::path get_bmi_path(const SourceDef &src) const {
        auto n = src.name;
        for (auto &c: n) if (c == ':') c = '-';
        n.append(".pcm");
        std::filesystem::path fname(n);
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


Version CompilerClang::get_clang_version(Config &cfg) {
    std::vector<ArgumentString> args;    
    append_arguments(args, {"--version"},{});
    auto p = Process::spawn(cfg.program_path, cfg.working_directory, args, Process::output);

    std::string banner((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());
    if (p.waitpid_status() != 0) {
        throw std::runtime_error("CLANG: unable to get version of compiler: " + cfg.program_path.string());
    }

    // Extract version from banner (e.g., "clang version 14.0.6")
    std::regex version_regex(R"((\d+\.\d+(?:\.\d+)?))");
    std::smatch match;
    if (std::regex_search(banner, match, version_regex)) {
        return match[1].str();
    } else {
        throw std::runtime_error("CLANG: unable to parse version from: " + banner);
    }


}



CompilerClang::CompilerClang(Config config) :_config((std::move(config))) {

    SystemEnvironment env = SystemEnvironment::current();
    _config.program_path = find_in_path(_config.program_path, env);


    _module_cache = _config.working_directory / "pcm";
    _object_cache = _config.working_directory / "obj";

    _version  = get_clang_version(_config);

    if (_version < Version("18.0")) {
        throw std::runtime_error("CLANG: version 18.0 or higher is required. Found: " + _version.to_string());
    }

    ThreadPool tp;
    tp.start(1);
    _preproc = initialize_preprocesor_using_gnu_compiler(_config.program_path, tp);

}

void CompilerClang::prepare_for_build() {
    std::filesystem::create_directories(_module_cache);
    std::filesystem::create_directories(_object_cache);
    if (std::find_if(_config.compile_options.begin(), _config.compile_options.end(), [&](const auto &opt){
        return opt.starts_with(stdcpp);
    }) == _config.compile_options.end()) {
        Log::warning("Missing C++ version (-std=c++xx) in command line options. This can cause misbehaviour during compilation!");
    }

}


int CompilerClang::link(std::span<const std::filesystem::path> objects, const std::filesystem::path &target) const {

    auto lstname = _object_cache/intermediate_file({ModuleType::source, "list", target}, ".lst");
    std::ofstream lst(lstname, std::ios::trunc|std::ios::out);
    if (!lst.is_open()) {
        Log::error("Failed to create list file: {}", lstname.string());
    }
    for (const auto &s: objects) {
         Log::debug("Link object {}", [&]{
            return s.string();
         });
         lst << s.u8string() << "\n";
    }
    lst.close();

    std::vector<ArgumentString> args = _config.link_options;
    append_arguments(args, {"@{}","-o","{}"}, {path_arg(lstname), path_arg(target)});
    int r =  invoke(_config, _config.working_directory, args);
    if (r) {
        dump_failed_cmdline(_config, _config.working_directory, args);
    }
    return r;
}

SourceScanner::Info CompilerClang::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    auto args = prepare_args(env,_config,'-');
    auto preproc = _preproc;
    auto info =  SourceScanner::scan_string(run_preprocess(preproc, args, env.working_dir, file));
    for (auto &s: info.required) {
        if (s.type == ModuleType::user_header) {
            s.name = (env.working_dir/s.name).lexically_normal().string();
        }
    }
    return info;
}



std::vector<ArgumentString> CompilerClang::build_arguments(bool precompile_stage,  const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const {

    std::vector<ArgumentString> args;
    if (!precompile_stage && (source.type == ModuleType::system_header || source.type == ModuleType::user_header )) {
        return args;
    }
    if (precompile_stage && (source.type == ModuleType::implementation || source.type == ModuleType::source )) {
        return args;
    }
    args = prepare_args(env,_config,'-');     
    append_arguments(args, {"-Xclang", "-fretain-comments-from-system-headers"},{});


    bool disable_experimental_warning = false;

    for (const auto &m: modules) {
        if (m.type != ModuleType::system_header 
            && m.type != ModuleType::user_header
            && m.path.parent_path() == _module_cache) {
               continue;
            }
        append_arguments(args, {"-fmodule-file={}"}, {path_arg(m.path)});
        disable_experimental_warning = true;    
    }

    if (disable_experimental_warning) {
        append_arguments(args,{"-Wno-experimental-header-units"},{});
    }

    switch (source.type) {

        case ModuleType::user_header: {
            result.interface = get_hdr_bmi_path(source);
            append_arguments(args,
                {"-fmodule-header=user", "-xc++-header", "--precompile", "{}", "-o", "{}"},
                {path_arg(source.path), path_arg(result.interface)});
            return args;
        }
        case ModuleType::system_header: {
            result.interface = get_hdr_bmi_path(source);
            append_arguments(args,
                {"-Wno-pragma-system-header-outside-header", "-fmodule-header=system", "-xc++-system-header", "--precompile", "{}", "-o", "{}"},
                {path_arg(source.path), path_arg(result.interface)});
            return args;
        }
        case ModuleType::partition:
        case ModuleType::interface: if (precompile_stage) {
            result.interface = get_bmi_path(source);
            append_arguments(args,
                {"-fprebuilt-module-path={}", "-xc++-module", "--precompile", "{}", "-o", "{}"},
                {path_arg(_module_cache), path_arg(source.path), path_arg(result.interface)});
            return args;
        }       
        default: break;    
    }

    result.object = get_obj_path(source);
    append_arguments(args,
        {"-fprebuilt-module-path={}","-c","{}","-o","{}"},
        {path_arg(_module_cache), path_arg(source.path), path_arg(result.object)});
    return args;

}


int CompilerClang::compile(const OriginEnv &env, 
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const {
    
    {
        auto args = build_arguments(true, env, source, modules, result);
        if (!args.empty()) {
            int r = invoke(_config, env.working_dir, args);
            if (r) {
                dump_failed_cmdline(_config, env.working_dir, args);;
                return r;   
            }
        }
    }
    {
        auto args = build_arguments(false, env, source, modules, result);
        if (!args.empty()) {
            int r = invoke(_config, env.working_dir, args);
            if (r) {
                dump_failed_cmdline(_config, env.working_dir, args);;
                return r;
            }
        }
    }
    return 0;
    
}

void CompilerClang::update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const  {

    CompileResult res;
    auto args = build_arguments(true, env, src, modules, res);
    if (!args.empty()) {
        cc.update(cc.record(env.working_dir, src.path,_config.program_path, std::move(args), std::move(res.interface)));
    } 
    args = build_arguments(false, env, src, modules, res);
    if (!args.empty()) {
        cc.update(cc.record(env.working_dir, src.path, _config.program_path,std::move(args), std::move(res.object)));
    } 
}

CompilerClang::SourceStatus CompilerClang::source_status(ModuleType t, const std::filesystem::path &file, std::filesystem::file_time_type tm) const
{
    //in case of clang we cannot detect a change in system header
    if (t == ModuleType::system_header) return SourceStatus::not_modified;
    return AbstractCompiler::source_status(t,file,tm);
}

std::unique_ptr<AbstractCompiler> create_compiler_clang(AbstractCompiler::Config cfg) {
    return std::make_unique<CompilerClang>(std::move(cfg));
}

void CompilerClang::update_link_command(CompileCommandsTable &cc,  
        std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const {
        std::vector<ArgumentString> args = _config.link_options;
        for (const auto &x: objects) args.push_back(path_arg(x));
        append_arguments(args, {"-o","{}"}, {path_arg(output)});
        cc.update(cc.record(_config.working_directory, {}, _config.program_path, std::move(args), output));
    }


 std::string  CompilerClang::preproc_for_test(const std::filesystem::path &file) const {
    auto args = _config.compile_options;
    auto preproc = _preproc;
    return run_preprocess(preproc, args, std::filesystem::current_path(), file);

 }