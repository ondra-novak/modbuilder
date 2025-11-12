#include "abstract_compiler.hpp"
#include "cli.hpp"
#include "app.hpp"
#include "module_database.hpp"
#include "utils/log.hpp"
#include "compilers/gcc/factory.hpp"
#include "compilers/clang/factory.hpp"
#include "compilers/msvc/factory.hpp"
#include "utils/arguments.hpp"
#include <filesystem>
#include <memory>

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
            std::cout << helptext << std::endl;
            return 0;
        }
        if (!succ) {
            clird.put_back();
            std::string tmp;
            for (auto *x = clird.text(); *x; ++x) tmp.push_back(static_cast<char>(*x));
            std::cerr << "Command line argument error near:" << tmp << std::endl;
            std::cerr << "Use -h for help" << std::endl;
            return 1;
        }

        auto compiler =create_compiler(settings);
        if (!compiler) {
            Log::error("Unknown compiler {}. Use -p to specify compiler.",  settings.compiler_path.string());
            std::cerr << "Use -h for help" << std::endl;
            return 1;
        }

        ModuleDatabase db;
        

        return 0;
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