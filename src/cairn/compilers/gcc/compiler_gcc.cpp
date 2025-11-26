#include "compiler_gcc.hpp"
#include "compile_commands_supp.hpp"
#include "factory.hpp"
#include "../../utils/log.hpp"
#include "../../utils/utf_8.hpp"
#include "../../gnu_compiler_setup.hpp"
#include "module_type.hpp"
#include <fstream>
#include <memory>
#include <future>
#include <utils/arguments.hpp>
#include <utils/process.hpp>
#include <regex>

Version CompilerGcc::get_gcc_version(Config &cfg) {
    std::vector<ArgumentString> args;
    append_arguments(args, {"--version"},{});
    auto p = Process::spawn(cfg.program_path, cfg.working_directory, args, Process::output);

    std::string banner((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());
    if (p.waitpid_status() != 0) {
        throw std::runtime_error("GCC: unable to get version of compiler: " + cfg.program_path.string());
    }

    // Extract version from banner (e.g., "clang version 14.0.6")
    std::regex version_regex(R"((\d+\.\d+(?:\.\d+)?))");
    std::smatch match;
    if (std::regex_search(banner, match, version_regex)) {
        return match[1].str();
    } else {
        throw std::runtime_error("GCC: unable to parse version from: " + banner);
    }
}



SourceScanner::Info CompilerGcc::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    auto args = prepare_args(env,_config,'-');
    auto preproc = _preproc;
    auto nfo =  SourceScanner::scan_string(run_preprocess(preproc, args, env.working_dir, file));
    auto paths =  preproc.get_include_paths();
    for (auto &s: nfo.required) {
        if (s.type == ModuleType::system_header) {
            for (const auto &x: paths) {
                std::filesystem::path candidate = (x/s.name).lexically_normal();
                std::error_code ec;
                if (std::filesystem::is_regular_file(candidate, ec)) {
                    s.name = s.name + "@"+candidate.string();
                    break;
                }
            }
        } else if (s.type == ModuleType::user_header) {
            s.name = s.name + "@" + (env.working_dir/s.name).lexically_normal().string();
        }
    }
    return nfo;
}



CompilerGcc::CompilerGcc(Config config):_config(std::move(config)) {

    SystemEnvironment env = SystemEnvironment::current();
    _config.program_path = find_in_path(_config.program_path, env);


    _module_cache = _config.working_directory / "gcm";
    _object_cache = _config.working_directory / "obj";
    _module_mapper = _config.working_directory / "modules.map";
    std::filesystem::create_directories(_module_cache);
    std::filesystem::create_directories(_object_cache);
    
    _version  = get_gcc_version(_config);

    if (_version < Version("14.0")) {
        throw std::runtime_error("GCC: version 14.0 or higher is required. Found: " + _version.to_string());
    }
    ThreadPool tp;
    tp.start(1);
    _preproc = initialize_preprocesor_using_gnu_compiler(_config.program_path, tp);
    
}

void CompilerGcc::prepare_for_build() {
    std::filesystem::create_directories(_module_cache);
    std::filesystem::create_directories(_object_cache);
}


static std::pair<std::string_view, std::string_view> separate_header_ref(std::string_view header_ref) {
    auto np = header_ref.find('@');
    if (np == header_ref.npos) 
        throw std::runtime_error(std::string("Following header reference is invalid for gcc (missing separator @):").append(header_ref));
    return {header_ref.substr(0,np), header_ref.substr(np+1)};
}

void CompilerGcc::initialize_module_map(std::span<const ModuleMapping> def)
{
    

    std::ofstream mapper(_module_mapper, std::ios::out|std::ios::trunc);
    if (!mapper.is_open())throw std::runtime_error("Can't create module mapper file: " + _module_mapper.string());    

    mapper << "$root " << _module_cache.string() << "\n";

    auto add_record = [&](const SourceDef &def, const auto & ... labels) {
        (mapper << ... << labels);
        auto gcm_path = intermediate_file(def,".gcm");
        mapper << " "<< gcm_path.string() << '\n';
    };

    for (const auto &x: def) {
        switch (x.type) {
            case ModuleType::user_header: {
                auto [name, path] = separate_header_ref(x.name);                
//                add_record(x, "./", name);
                add_record(x, (x.work_dir/"xxx").parent_path().string(), "/", name); //ensure, that x.work_dir is dir
            }break;
            case ModuleType::system_header: {                
                auto [name, path] = separate_header_ref(x.name);                
                add_record(x, path);
            }break;
            case ModuleType::interface:
            case ModuleType::partition: 
                add_record(x, x.name) ; 
                break;
            default: 
                continue;
        }
    }
}

int CompilerGcc::link(std::span<const std::filesystem::path> objects, const std::filesystem::path & target) const
{
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

std::filesystem::path CompilerGcc::create_adhoc_mapper(const SourceDef &src) const {
    auto mapper_file = _module_cache/intermediate_file(src, ".map");
    auto gcm_path = intermediate_file(src, ".gcm");

    std::ofstream f(mapper_file, std::ios::out|std::ios::trunc);
    f << "$root " << _module_cache.string() << "\n";
    switch (src.type) {
        case ModuleType::system_header:{                
                auto [name, path] = separate_header_ref(src.name);                
                f << path << " " << gcm_path.string() << "\n";                
                return mapper_file;
        }
        case ModuleType::user_header:{                
                auto [name, path] = separate_header_ref(src.name);                
                f << "./" << name <<  " " << gcm_path.string() << "\n";                                
                return mapper_file;
        }
        default:
            return _module_mapper;
    }
}


std::vector<ArgumentString> CompilerGcc::build_arguments(const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> ,
        CompileResult &result) const {

    std::vector<ArgumentString> args;
    args = prepare_args(env,_config,'-');                                                
    append_arguments(args, {"-fmodules-ts", "-fmodule-mapper={}"},{path_arg(create_adhoc_mapper(source))});

    switch (source.type) {

        case ModuleType::user_header: {
            auto [name,path] = separate_header_ref(source.name);
            result.interface = _module_cache/intermediate_file(source, ".gcm");
            append_arguments(args,{ "-xc++-header", "-c", "{}"},{string_arg(name)});
            return args;
        }
        case ModuleType::system_header: {
            auto [name,path] = separate_header_ref(source.name);
            result.interface = _module_cache/intermediate_file(source, ".gcm");
            append_arguments(args,{ "-xc++-system-header", "-c", "{}"},{string_arg(name)});
            return args;
        }
        case ModuleType::partition:
        case ModuleType::interface:  {
            result.interface = _module_cache/intermediate_file(source, ".gcm");
            result.object = _object_cache/intermediate_file(source, ".o");
            append_arguments(args,{ "-xc++", "-c", "{}", "-o", "{}"},{path_arg(source.path), path_arg(result.object)});
            return args;        
        }
        default: {
            result.object = _object_cache/intermediate_file(source, ".o");
            append_arguments(args,{ "-xc++", "-c", "{}", "-o", "{}"},{path_arg(source.path), path_arg(result.object)});
            return args;        
        }
    }
}

int CompilerGcc::compile(const OriginEnv &env, 
        const SourceDef &source,
        std::span<const SourceDef> modules,
        CompileResult &result) const {
    {
        auto args = build_arguments( env, source, modules, result);
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


std::unique_ptr<AbstractCompiler> create_compiler_gcc(AbstractCompiler::Config cfg) {
    return std::make_unique<CompilerGcc>(std::move(cfg));
}


void CompilerGcc::update_compile_commands(CompileCommandsTable &cc,  const OriginEnv &env, 
                const SourceDef &src, std::span<const SourceDef> modules) const  {

    CompileResult res;
    auto args = build_arguments( env, src, modules, res);    
    auto out = res.interface.empty()?std::move(res.object):std::move(res.interface);
    cc.update(cc.record(env.working_dir, src.path, _config.program_path, std::move(args), std::move(out)));
}

AbstractCompiler::SourceStatus CompilerGcc::source_status(ModuleType t,
        const std::filesystem::path &file, std::filesystem::file_time_type tm) const {
    if (is_header_module(t)) {
        std::string s = file.string();
        auto [name, path] =  separate_header_ref(s);
        return AbstractCompiler::source_status(t, path, tm);
    } else {
        return AbstractCompiler::source_status(t, file, tm);
    }
}

void CompilerGcc::update_link_command(CompileCommandsTable &cc,  
        std::span<const std::filesystem::path> objects, const std::filesystem::path &output) const {
        std::vector<ArgumentString> args = _config.link_options;
        for (const auto &x: objects) args.push_back(path_arg(x));
        append_arguments(args, {"-o","{}"}, {path_arg(output)});
        cc.update(cc.record(_config.working_directory, {}, _config.program_path, std::move(args), output));
    }

