#include "utils/arguments.hpp"
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
    std::filesystem::path source_file_path = {};
    std::filesystem::path compiler_path = {};
    std::vector<ArgumentString> compiler_arguments = {};
    std::vector<ArgumentString> linker_arguments = {};
    std::vector<ArgumentString> lib_arguments =  {};
    int threads = 1;
    Mode mode = compile_and_link;
    bool show_help = false;
    bool recompile = false;
    bool keep_going = false;
    bool dry_run = false;

};

bool parse_cmdline(AppSettings &settings, CliReader<ArgumentString::value_type> &cli);
extern const std::string_view helptext;