#include "utils/arguments.hpp"
#include "compile_target.hpp"
#include <filesystem>
#include <vector>



struct AppSettings {

    enum Mode {
        compile_and_link,
        compile_only,
        link_only
    };

    ArgumentString compiler_type = {};
    std::filesystem::path compile_commands_json = {};
    std::filesystem::path env_file_json = {};
    std::filesystem::path working_directory_path = {};
    std::filesystem::path compiler_path = {};
    std::vector<ArgumentString> compiler_arguments = {};
    std::vector<ArgumentString> linker_arguments = {};
    std::vector<ArgumentString> lib_arguments =  {};
    std::vector<CompileTarget> targets = {};
    std::filesystem::path scan_file = {};
    unsigned int threads = 1;
    Mode mode = compile_and_link;
    bool show_help = false;
    bool recompile = false;
    bool keep_going = false;
    bool drop_database = false;

};

bool parse_cmdline(AppSettings &settings, CliReader<ArgumentString::value_type> &cli);
std::string_view get_help();