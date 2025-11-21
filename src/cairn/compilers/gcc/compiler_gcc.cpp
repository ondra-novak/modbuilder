#include "compiler_gcc.hpp"
#include "factory.hpp"
#include "../../utils/log.hpp"
#include "../../utils/temp_file.hpp"
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


static std::vector<std::filesystem::path> extract_include_path(std::string text, const std::filesystem::path working_dir) {    
    std::vector<std::filesystem::path> res;

    std::string ln;

    std::istringstream strm(std::move(text));
    bool record = false;
    while (!!strm) {
        std::getline(strm, ln);
        std::string_view lnw(ln);
        while (!lnw.empty() && isspace(lnw.front())) lnw = lnw.substr(1);
        while (!lnw.empty() && isspace(lnw.back())) lnw = lnw.substr(0,lnw.size()-1);
        if (record) {
            if (lnw == "End of search list.") break;
            res.emplace_back((working_dir/lnw).lexically_normal());
        } else if (lnw == "#include <...> search starts here:") {
            record = true;
        }
    }
    return res;
}



SourceScanner::Info CompilerGcc::scan(const OriginEnv &env, const std::filesystem::path &file) const
{
    auto pp = preprocess(env, file);
    auto nfo = SourceScanner::scan_string(pp.first);
    auto paths =  extract_include_path(pp.second, env.working_dir);
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


std::pair<std::string,std::string> CompilerGcc::preprocess(const OriginEnv &env,const std::filesystem::path &file) const {

    auto args = prepare_args(env,_config,'-');

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
    append_arguments(args, {"-xc++", "-v", "-E", "{}"}, {path_arg(file)});


    Process p = Process::spawn(_config.program_path, env.working_dir, args, Process::output|Process::error);

    std::promise<std::string> errprom;
    _helper.push([&]() noexcept {
        try {
            std::string err((std::istreambuf_iterator<char>(*p.stderr_stream)),
                            std::istreambuf_iterator<char>());
            errprom.set_value(std::move(err));
        } catch (...) {
            errprom.set_exception(std::current_exception());
        }
    });

    std::string out((std::istreambuf_iterator<char>(*p.stdout_stream)),
                     std::istreambuf_iterator<char>());

    auto err =errprom.get_future().get();
    if (p.waitpid_status()) {
        std::cerr << err << std::endl;
        dump_failed_cmdline(_config, env.working_dir, args);
    }
    return {std::move(out),std::move(err)};
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
    _helper.start(1);
    
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
                add_record(x, "./", name);
                add_record(x, x.work_dir.string(), "/", name);
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
    OutputTempFile tmpf;
    std::ostream &f = tmpf.create();
    for (const auto &s: objects) {
         Log::debug("Link object {}", [&]{
            return s.string();
         });
         auto str = s.u8string();
         f.write(reinterpret_cast<const char *>(str.data()), str.size());
         f.put('\n');
    }
    auto tmppath = tmpf.commit();

    std::vector<ArgumentString> args = _config.link_options;
    append_arguments(args, {"@{}","-o","{}"}, {path_arg(tmppath), path_arg(target)});
    int r =  invoke(_config, _config.working_directory, args);
    if (r) {
        dump_failed_cmdline(_config, _config.working_directory, args);
    }
    return r;
}

std::vector<ArgumentString> CompilerGcc::build_arguments(const OriginEnv &env,
        const SourceDef &source,
        std::span<const SourceDef> ,
        CompileResult &result) const {

    std::vector<ArgumentString> args;
    args = prepare_args(env,_config,'-');                                                
    append_arguments(args, {"-fmodules-ts", "-fmodule-mapper={}"},{path_arg(_module_mapper)});

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

bool CompilerGcc::generate_compile_command(
        const OriginEnv &env,
        const SourceDef &src,
        std::span<const SourceDef> modules,
        std::vector<ArgumentString> &result) const {

    CompileResult dummy;
    result = build_arguments(env, src, modules, dummy);
    if (result.empty()) return false;
    result.insert(result.begin(), path_arg(_config.program_path));
    return true;

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

