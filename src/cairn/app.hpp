#include "abstract_compiler.hpp"
#include "origin_env.hpp"
#include "json/value.h"
#include <filesystem>
class App {
public:





    std::filesystem::path source_file;
    json::value this_project;
    POriginEnv this_orig;
    std::unique_ptr<AbstractCompiler> compiler;
    std::string compiler_profile;
    std::filesystem::path project_path;
    ArgumentString compiler_specf;
    unsigned int threads;

    int run();


    bool arg_show_help() ;
    bool arg_set_threads(int j) {
        threads = j;
        return true;
    }
    bool arg_set_compiler(std::string cmp) {
        compiler_profile = std::move(cmp);        
        return true;
    }
    bool arg_set_project(std::filesystem::path p) {
        project_path = std::move(p);
        return true;
    }
        
    bool arg_set_compile_file(std::filesystem::path p) {
        source_file = std::move(p);
        return false;
    }


};
