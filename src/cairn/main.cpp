#include "abstract_compiler.hpp"
#include "builder.hpp"
#include "cli.hpp"
#include "module_database.hpp"
#include "utils/log.hpp"
#include "compilers/gcc/factory.hpp"
#include "compilers/clang/factory.hpp"
#include "compilers/msvc/factory.hpp"
#include "utils/arguments.hpp"
#include <json/parser.h>
#include <filesystem>
#include <memory>
#include <fstream>

static constexpr auto gcc_type_1 = ArgumentConstant("gcc");
static constexpr auto gcc_type_2 = ArgumentConstant("g++");
static constexpr auto gcc_type_3 = ArgumentConstant("gnu");
static constexpr std::array<ArgumentStringView,3> gcc_types= {gcc_type_1, gcc_type_2, gcc_type_3};
static constexpr auto clang_type_1 = ArgumentConstant("clang");
static constexpr auto clang_type_2 = ArgumentConstant("clang++");
static constexpr auto clang_type_3 = ArgumentConstant("llvm");
static constexpr std::array<ArgumentStringView,3> clang_types = {clang_type_1, clang_type_2, clang_type_3};
static constexpr auto msvc_type_1 = ArgumentConstant("msvc");
static constexpr auto msvc_type_2 = ArgumentConstant("msc");
static constexpr auto msvc_type_3 = ArgumentConstant("cl");
static constexpr auto msvc_type_4 = ArgumentConstant("cl.exe");
static constexpr std::array<ArgumentStringView,4> msvc_types = {msvc_type_1, msvc_type_2, msvc_type_3, msvc_type_4};


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
        } else if (contains(settings.compiler_type, msvc_types)) {
            factory = &create_compiler_msvc;
        } else {
            return {};
        }
    } else {
        auto exec_name = settings.compiler_path.filename().string();
        
        std::transform(exec_name.begin(), exec_name.end(), exec_name.begin(),
               [](unsigned char c){ return std::tolower(c); });

        if (exec_name == "cl" || exec_name == "cl.exe") {
            factory = &create_compiler_msvc;            
        } else if (exec_name.rfind("clang") != exec_name.npos) {
            factory = &create_compiler_clang;            
        } else if (exec_name.rfind("gcc") != exec_name.npos || exec_name.rfind("g++") != exec_name.npos) {
            factory = &create_compiler_gcc;
        } else {
            return {};
        }        
    }

    AbstractCompiler::Config cfg;

    cfg.program_path = settings.compiler_path;

    for (const auto& dir : get_path_entries()) {
        std::filesystem::path candidate = dir / cfg.program_path;
        if (std::filesystem::exists(candidate) &&
            std::filesystem::is_regular_file(candidate)) {
                cfg.program_path = std::move(candidate);
                break;
        }
    }
          
    cfg.compile_options  = std::move(settings.compiler_arguments);
    cfg.link_options  = std::move(settings.linker_arguments);
    cfg.working_directory = settings.working_directory_path;
    std::filesystem::create_directories(cfg.working_directory);
    return factory(std::move(cfg));
}
ModuleDatabase load_database_from_file(std::istream &f) {
    std::vector<char> data;
    ModuleDatabase db;
    std::copy(std::istreambuf_iterator<char>(f),
              std::istreambuf_iterator<char>(), 
              std::back_inserter(data));
    json::value jdata;
    json::parser_t p;
    p.parse(data.begin(), data.end(), jdata);
    return ModuleDatabase{std::move(jdata)};
}

ModuleDatabase load_database(const std::filesystem::path &path) {
    std::ifstream f(path);
    if (!f) return {};
    else return load_database_from_file(f);
}

void save_database(const ModuleDatabase &db, const std::filesystem::path &path) {
    auto s = db.export_db().to_json();
    std::ofstream f(path, std::ios::out|std::ios::trunc);
    f << s << "\n";
}


int run_just_scan(AbstractCompiler &compiler, const std::filesystem::path &file) {
    auto print_list = [&](const std::vector<SourceScanner::Reference> &list) {
        
        for (const auto &x: list) {
            std::cout << " - type: " << to_string(x.type) << "\n";
            std::cout << "   name: " << x.name << "\n";
        }
    };

    auto info = compiler.scan(OriginEnv::default_env(), file);
    std::cout << "module_name: " << info.name << "\n";
    std::cout << "type: " << to_string(info.type) << "\n";
    std::cout << "exports:\n";
    print_list(info.exported);
    std::cout << "imports:\n";
    print_list(info.required);
    return 0;
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

        std::string db_name (compiler->get_compiler_name());
        db_name.append(".db");
        auto db_path = settings.working_directory_path/db_name;

        auto db = settings.drop_database?ModuleDatabase():load_database(db_path);

        POriginEnv default_env = std::make_shared<OriginEnv>(OriginEnv::default_env());

        db.check_for_modifications(*compiler);
        if (!settings.env_file_json.empty()) {
            db.add_origin(settings.env_file_json, *compiler);
        }
        for (const auto &ts: settings.targets) {
            db.add_file( ts.source, *compiler);
        }

        if (settings.recompile) db.recompile_all();
        else db.check_for_recompile();

        auto plan = db.create_build_plan(*compiler, *default_env, 
                    settings.targets, 
                    settings.recompile, 
                    !settings.lib_arguments.empty());
                

        auto threads = settings.threads;
        compiler->prepare_for_build();
        bool use_build_system = compiler->initialize_build_system({threads, settings.keep_going});
        if (use_build_system) threads = 1;
        std::vector<AbstractCompiler::ModuleMapping> module_map;
        db.extract_module_mapping(plan, module_map);
        compiler->initialize_module_map(module_map);
        ThreadPool tp;
        tp.start(threads);
        bool ret = Builder::build(tp, plan, settings.keep_going).get();

        if (!settings.compile_commands_json.empty()) {
            CompileCommandsTable cctable;
            cctable.load(settings.compile_commands_json);
            for (auto &item: plan)item.action.add_to_cctable(cctable);            
            cctable.save(settings.compile_commands_json);
        }

        if (db.is_dirty()) save_database(db, db_path);

        
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