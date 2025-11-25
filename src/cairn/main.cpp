#include "abstract_compiler.hpp"
#include "build_plan.hpp"
#include "builder.hpp"
#include "cli.hpp"
#include "compile_commands_supp.hpp"
#include "module_database.hpp"
#include "module_resolver.hpp"
#include "module_type.hpp"
#include "origin_env.hpp"
#include "utils/log.hpp"
#include "compilers/gcc/factory.hpp"
#include "compilers/clang/factory.hpp"
#include "compilers/msvc/factory.hpp"
#include "utils/arguments.hpp"
#include "utils/thread_pool.hpp"
#include "utils/utf_8.hpp"
#include <exception>
#include <filesystem>
#include <iterator>
#include <memory>
#include <fstream>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <variant>

static constexpr auto gcc_type_1 = ArgumentConstant("gcc");
static constexpr auto gcc_type_2 = ArgumentConstant("g++");
static constexpr auto gcc_type_3 = ArgumentConstant("gnu");
static constexpr std::array<ArgumentStringView,3> gcc_types= {gcc_type_1, gcc_type_2, gcc_type_3};
static constexpr auto clang_type_1 = ArgumentConstant("clang");
static constexpr auto clang_type_2 = ArgumentConstant("clang++");
static constexpr auto clang_type_3 = ArgumentConstant("llvm");
static constexpr std::array<ArgumentStringView,3> clang_types = {clang_type_1, clang_type_2, clang_type_3};
#ifdef _WIN32
static constexpr auto msvc_type_1 = ArgumentConstant("msvc");
static constexpr auto msvc_type_2 = ArgumentConstant("msc");
static constexpr auto msvc_type_3 = ArgumentConstant("cl");
static constexpr auto msvc_type_4 = ArgumentConstant("cl.exe");
static constexpr std::array<ArgumentStringView,4> msvc_types = {msvc_type_1, msvc_type_2, msvc_type_3, msvc_type_4};
#endif

static std::unique_ptr<AbstractCompiler> create_compiler(const AppSettings &settings) {
    auto contains = [](ArgumentStringView what, const auto &where) {
        auto iter = std::find(where.begin(), where.end(), what);
        return iter != where.end();
    };
    std::unique_ptr<AbstractCompiler> (*factory)(AbstractCompiler::Config) = nullptr;

    if (!settings.compiler_type.empty()) {
        if (contains(settings.compiler_type, gcc_types)) {
            factory = &create_compiler_gcc;
        } else if (contains(settings.compiler_type, clang_types)) {
            factory = &create_compiler_clang;
#ifdef _WIN3
        } else if (contains(settings.compiler_type, msvc_types)) {
            factory = &create_compiler_msvc;
#endif            
        } else {
            return {};
        }
    } else {
        auto exec_name = settings.compiler_path.filename().string();
        
        std::transform(exec_name.begin(), exec_name.end(), exec_name.begin(),
               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

#ifdef _WIN3
        if (exec_name.rfind("cl.exe") != exec_name.npos) {
            factory = &create_compiler_msvc;            
        } else
#endif  
        if (exec_name.rfind("clang") != exec_name.npos) {
            factory = &create_compiler_clang;            
        } else if (exec_name.rfind("gcc") != exec_name.npos || exec_name.rfind("g++") != exec_name.npos) {
            factory = &create_compiler_gcc;
        } else {
            return {};
        }        
    }

    AbstractCompiler::Config cfg;

    cfg.program_path = settings.compiler_path;
          
    cfg.compile_options  = std::move(settings.compiler_arguments);
    cfg.link_options  = std::move(settings.linker_arguments);
    cfg.working_directory = settings.working_directory_path;
    std::filesystem::create_directories(cfg.working_directory);
    return factory(std::move(cfg));
}

void  load_database(ModuleDatabase &db, const std::filesystem::path &path) {
    std::ifstream f(path, std::ios::in|std::ios::binary);
    if (!!f) {
        try {
            db.import_database(f);
        } catch (std::exception &e) {
            Log::warning("Database is corrupted. Rebuilding: {}", e.what());
            db.clear();
        }
    }
}


void save_database_binary(const ModuleDatabase &db, const std::filesystem::path &path) {
    std::ofstream f(path, std::ios::out|std::ios::trunc|std::ios::binary);
    db.export_database(f);

}


int run_just_scan(AbstractCompiler &compiler, const std::filesystem::path &file) {

    std::error_code ec;
    if (!std::filesystem::is_regular_file(file,ec)) {
        std::cerr << "File " << file << " cannot be scanned: " << ec.message() << std::endl;
        return ec.value();
    }

    auto print_list = [&](const std::vector<SourceScanner::Reference> &list) {
        
        if (list.empty()) {
            std::cout << " []\n";            
        } else {
            std::cout << "\n";
            for (const auto &x: list) {
                std::cout << " - type: " << to_string(x.type) << "\n";
                std::cout << "   name: " << x.name << "\n";
            }
        }
    };

    auto dir = file.parent_path();
    auto org = OriginEnv{dir,dir, 0,{}, {}, {}};

    auto info = compiler.scan(org, file);
    std::cout << "---\n";
    std::cout << "module_name: " << info.name << "\n";
    std::cout << "type: " << to_string(info.type) << "\n";
    std::cout << "exports:";
    print_list(info.exported);
    std::cout << "imports:";
    print_list(info.required);
    return 0;
}

static void list_modules(const ModuleDatabase &db, const std::vector<AbstractCompiler::ModuleMapping>  &map) {
    auto filter = [&](auto fn) -> std::vector<std::string_view> {
        std::vector<std::string_view> out;
        for (const auto &x: map) if (fn(x.type)) out.push_back(x.name);
        std::sort(out.begin(), out.end());
        return out;
    };

    auto modules = filter([](ModuleType t){return !is_header_module(t);});
    auto system_headers = filter([](ModuleType t){return t == ModuleType::system_header;});
    auto user_headers = filter([](ModuleType t){return t == ModuleType::user_header;});

    auto print_list = [] (const auto &x) {
        if (x.empty()) std::cout << "[]\n"; else {
            std::cout << "\n";
            for (const auto &y:x) std::cout << " - " << y << "\n";
        }
    };

    std::cout << "---\n";
    std::cout << "modules:";
    print_list(modules);
    std::cout << "system_headers:";
    print_list(system_headers);
    std::cout << "user_headers:";
    print_list(user_headers);

    std::unordered_set<POriginEnv> origins;
    for (const auto &x: map) {
        auto f = db.find(x.path);
        origins.insert(f->origin);
    }

    std::cout << "origins:";
    if (origins.empty()) std::cout << "[]";
    std::cout << "\n";
    for (const auto &o: origins) {
        std::cout << " - " << o->config_file << std::endl;
    }


}

static void generate_makefile(const BuildPlan<ModuleDatabase::CompileAction> &plan,            
            std::filesystem::path output            
        ) {
    auto cur_dir = output.parent_path();;
    std::ofstream mk(output, std::ios::out|std::ios::trunc);
    std::size_t idx = 0;
    std::unordered_set<unsigned int> all_targets;
    mk << ".PHONY: all";
    for (idx = 0; idx < plan.get_plan().size(); ++idx) {
        mk << " t_" << idx;
        all_targets.insert(idx);
    }    
    for (idx = 0; idx < plan.get_plan().size(); ++idx) {
        for (const auto &x: plan.get_plan()[idx].dependencies) {
            all_targets.erase(x);
        }
    }
    
    std::unordered_set<std::filesystem::path> workdirs;

    mk << "\nall:";
    for (auto &x: all_targets) {
        mk << " t_"<< x;
    }
    mk << "\n";
    mk << "\n";
    idx = 0;
    ArgumentString srchpath = path_arg((cur_dir/".").lexically_normal());
    auto remove_abs_path = [&](ArgumentString arg) {
        auto np = arg.find(srchpath);
        while (np != arg.npos) {
            arg = arg.substr(0,np) + arg.substr(np+srchpath.size());
            np = arg.find(srchpath);
        }
        return arg;
    };
    for (const auto &p:plan) {
        CompileCommandsTable cctmp;
        p.action.add_to_cctable(cctmp);
        mk << "t_" << idx << ":";
        for (auto &d: p.dependencies) {
            mk << " t_" << d;
        }
        mk << "| workdir \n";
        for (auto &[k,v]: cctmp._table) {
            
            ArgumentString arg = remove_abs_path(v.command);
            mk << "\t";
            to_utf8(arg.begin(), arg.end(), std::ostreambuf_iterator<char>(mk));
            mk << "\n";
            workdirs.insert(v.output.parent_path());
        }
        ++idx;
        mk << "\n";
    }
    mk << "\nworkdir:\n";
    for (auto &w: workdirs) {
        auto p = remove_abs_path(path_arg(w));
        mk << "\tmkdir -p ";
        to_utf8(p.begin(), p.end(), std::ostreambuf_iterator<char>(mk));
        mk << "\n";
    }
}



int tmain(int argc, ArgumentString::value_type *argv[]) {

    try {
        AppSettings settings;
        auto curdir = std::filesystem::current_path();

        if (argc < 2) {
            std::cerr << "Requires command line arguments. Use -h for help\n";
            return 1;
        }

        auto clird = CliReader<ArgumentString::value_type>(argc-1, argv+1);
        settings.working_directory_path = curdir/".build";    


        bool succ = parse_cmdline(settings, clird);
        if (settings.show_help) {
            std::cout << get_help() << "\n";
            return 0;
        }
        if (!succ) {
            clird.put_back();
            std::string tmp;
            for (auto *x = clird.text(); *x; ++x) tmp.push_back(static_cast<char>(*x));
            std::cerr << "Command line argument error near:" << tmp << "\n";
            std::cerr << "Use -h for help" << "\n";
            return 1;
        }

        auto compiler =create_compiler(settings);
        if (!compiler) {
            Log::error("Unknown compiler {}. Use -p to specify compiler.",  settings.compiler_path.string());
            std::cerr << "Use -h for help" << "\n";
            return 1;
        }

        if (!settings.scan_file.empty()) {
            return run_just_scan(*compiler, settings.scan_file);            
        }

        auto db_path = settings.working_directory_path/"modules.db";

        ModuleDatabase db;
        if (!settings.drop_database) load_database(db, db_path);
        if (!db.check_database_version(settings.compiler_path, settings.compiler_arguments)) {
            Log::verbose("Settings has been changed, rebuilding");
        }

        POriginEnv default_env = std::make_shared<OriginEnv>(OriginEnv::default_env());

        db.check_for_modifications(*compiler);
        auto targets = settings.targets;
        if (!settings.env_file_json.empty()) {
            auto r = ModuleResolver::loadMap(settings.env_file_json);
            db.add_origin(r, *compiler);
            if (!r.targets.empty() && targets.empty()) { 
                targets = r.targets;
            }
        }
        for (const auto &ts: targets) {
            db.add_file( ts.source, *compiler);
        }

        if (!settings.generate_makefile.empty()) {
            db.recompile_all();
            auto mplan = db.create_build_plan(*compiler, *default_env, 
                    targets, false,  !settings.lib_arguments.empty());
            compiler->dry_run(true);
            ThreadPool tp;
            tp.start(1);
            Builder::build(tp, mplan, true).get();
            generate_makefile(mplan, settings.generate_makefile);            
            return 0;
        }


        if (settings.recompile || settings.list) db.recompile_all();
        else db.check_for_recompile();

        auto plan = db.create_build_plan(*compiler, *default_env, 
                    targets, false,  !settings.lib_arguments.empty());

    
                
        std::vector<AbstractCompiler::ModuleMapping> module_map;
        if (settings.list) {
            db.extract_module_mapping(plan, module_map);
            list_modules(db,module_map);
            return 0;
        }


        auto threads = settings.threads;
        if (threads == 0) threads = std::thread::hardware_concurrency();

        compiler->prepare_for_build();
        bool use_build_system = compiler->initialize_build_system({threads, settings.keep_going});
        if (use_build_system) threads = 1;
        db.extract_module_mapping(plan, module_map);
        compiler->initialize_module_map(module_map);
        ThreadPool tp;
        tp.start(threads);
        bool ret = Builder::build(tp, plan, settings.keep_going).get();

        if (!settings.compile_commands_json.empty()) {
            CompileCommandsTable cctable;
            cctable.load(settings.compile_commands_json);
            db.update_compile_commands(cctable, *compiler);
            cctable.save(settings.compile_commands_json);
        }

        if (db.is_dirty()) {
            save_database_binary(db, db_path);
        }

        if (ret) {
            Log::verbose("Done");
        } else {
            Log::error("Failed");
        }
        return ret?0:1;
    } catch (std::exception &e) {
        Log::error("{}", e.what());
        return 1;
    }

}


#ifdef _WIN32
int wmain(int argc, wchar_t *argv[]) {
    return tmain(argc, argv);
}
#else
int main(int argc, char *argv[]) {
    return tmain(argc,reinterpret_cast<char8_t **>(argv));
}
#endif