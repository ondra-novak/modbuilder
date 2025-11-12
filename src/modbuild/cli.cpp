
#include "cli.hpp"
#include "utils/arguments.hpp"
#include "utils/log.hpp"
#include <filesystem>

static constexpr auto compile_flag = ArgumentConstant("--compile:");
static constexpr auto link_flag = ArgumentConstant("--link:");
static constexpr auto lib_flag = ArgumentConstant("--lib:");

enum class Stage {
    common,
    compile,
    link,
    lib
};

const std::string_view helptext = R"help(Usage:

modbuild [...switches...] <file.cpp>  <compiler> [...compiler/linker..flags...]

Switches
========
-jN       specify count of thread
-p<type>  select compiler type: gcc|clang|msvc
-c<path>  generate compile_commands.json (path=where)
-f<file>  specify environment file (modules.json) for this build
-b<dir>   specify build directory
-C        compile only (doesn't run linker)
-L        link only (requires compiled files in build directory)
-k        keep going
-r        recompile whole database even if modules are not referenced
-y        dry-run don't compile (but can generate compile_commans.json)
-s        output only errors (silent)
-d        debug mode (output everyting)

file.cpp  specifies path to file to compile. If -W is used, the
            then it specifies pathname to modules-like json file
            to compile.

compiler  path to compiler's binary. PATH is used to search binary

Compiler arguments
==================
Any arguments here are copied to command-line when compiler is invoke
However, there are switches that have special meaning (are not copied)
Note these switches end with colon

--compile:  following arguments are used only during compilation phase
--link:     following arguments are used only during link phase
--lib:      produces library (will not link), following arguments are
            used for librarian

Example: gcc -DSPECIAL -I/usr/local/include --compile: -O2 -march=native --link: -o example -lthread

)help";

bool parse_cmdline(AppSettings &settings, CliReader<ArgumentString::value_type> &cli) {
    ArgumentString file;
    auto curdir = std::filesystem::current_path();
    while(true) {
        auto p = cli.next();
        if (p.is_end) return false;
        if (p.is_text) {
            settings.source_file_path = (curdir/p.text).lexically_normal();
            break;
        }
        if (p.is_long_sw) return false;

        switch (p.short_sw) {
            default: return false;
            case 'j': settings.threads = cli.number();break;
            case 'p': settings.compiler_type = cli.text();break;
            case 'c': settings.compile_commands_json = (curdir/cli.text()).lexically_normal();break;
            case 'f': settings.env_file_json = (curdir/cli.text()).lexically_normal();break;
            case 'b': settings.working_directory_path = (curdir/cli.text()).lexically_normal();break;
            case 'r': settings.recompile = true;break;
            case 'C': settings.mode = AppSettings::compile_only;break;
            case 'L': settings.mode = AppSettings::link_only;break;
            case 's': Log::set_level(Log::Level::error);break;
            case 'd': Log::set_level(Log::Level::debug);break;            
            case 'h': settings.show_help = true;break;
            case 'k': settings.keep_going = true;break;
        }
    }
    {
        auto p = cli.next();
        if (p.is_end) return false;
        settings.compiler_path = p.text;
    }
    Stage stage = Stage::common;
    while (cli) {
        ArgumentString a = cli.text();
        if (a == compile_flag) {
            stage = Stage::compile;
        } else if (a == link_flag) {
            stage = Stage::link;
        } else if (a == lib_flag) {
            stage = Stage::lib;
        } else {
            switch (stage) {
                case Stage::common:
                    settings.linker_arguments.push_back(a);
                    settings.compiler_arguments.push_back(std::move(a));
                    break;
                case Stage::compile:
                    settings.compiler_arguments.push_back(std::move(a));
                    break;
                case Stage::link:
                    settings.linker_arguments.push_back(std::move(a));
                    break;
                case Stage::lib:
                    settings.lib_arguments.push_back(std::move(a));
                    break;                    
            }
        }
    }
    return true;
}