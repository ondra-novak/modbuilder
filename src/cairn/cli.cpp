export module cairn.cli;

import cairn.utils.arguments;
import cairn.utils.log;
import cairn.version;
import cairn.compile_target;
import <filesystem>;
import <vector>;

export struct AppSettings {

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
    std::filesystem::path generate_makefile = {};
    std::filesystem::path preproc_file = {};
    unsigned int threads = 1;
    Mode mode = compile_and_link;
    bool show_help = false;
    bool recompile = false;
    bool keep_going = false;
    bool drop_database = false;
    bool list = false;

};

export bool parse_cmdline(AppSettings &settings, CliReader<ArgumentString::value_type> &cli);
export std::string_view get_help();

static constexpr auto compile_flag = ArgumentConstant("--compile:");
static constexpr auto link_flag = ArgumentConstant("--link:");
static constexpr auto lib_flag = ArgumentConstant("--lib:");

enum class Stage {
    common,
    compile,
    link,
    lib
};

constexpr std::string_view appname("Cairn");
constexpr std::string_view licence("Copyright (c) 2025 Ondrej Novak (https://github.com/ondra-novak)\n"
                                   "This software is released under the MIT License. https://opensource.org/licenses/MIT");

constexpr std::string_view helptext = R"help(Usage:

cairn [...switches...] <output1=file1.cpp> [<output2=file2.cpp>... ] <compiler> [...compiler/linker..flags...]

Switches
========
-jN       specify count of thread
-p<type>  select compiler type: gcc|clang|msvc
-c<path>  generate compile_commands.json (path=where)
-f<file>  specify environment file (modules.yaml) for this build
-b<dir>   specify build directory
-C        compile only (doesn't run linker)
-L        link only (requires compiled files in build directory)
-k        keep going
-r        recompile all related files
-R        drop database, rescan and recompile
-s        output only errors (silent)
-d        debug mode (output everyting)
-l --list don't compile, just output list of all referenced modules and headers
-M<file>  don't compile, create makefile containing all build steps needs to 
          create targets (works well with clang++ v18+) 

outputN   specifies path/name of output executable
fileN.cpp specifies path/name of main file for this executable

          you can specify multiple targets 
          target1=file1 target2=file2 target3=file3 ....
          there are no spaces before and after '='


compiler  path to compiler's binary. PATH is used to search binary

Compiler arguments
==================
All arguments listed here are copied to the compiler command line when it is run.
However, there are switches that have special meanings and are not copied. These
switches end with a colon.

--compile:  following arguments are used only during compilation phase
--link:     following arguments are used only during link phase
--lib:      produces library (will not link), following arguments are
            used by librarian. This also activates library build 

Example: gcc -DSPECIAL -I/usr/local/include --compile: -O2 -march=native --link: -o example -lthread

Module discovery
================
A modules.yaml file can be defined in each module directory. The file is in YAML format 
with the following fields

files:          list of files involved in the build (optional)
    -
    -
includes:       list of paths for searching headers (optional)
    -
    -
options:        additional compile options (each on separate line)
    -
    -
prefixes:       mapping prefix->paths              
    pfx1:       All modules with prefix "pfx1" can be located 
        -       on these paths
        -
    pfx2: xxx   example with single path
    "":         all other modules 
        -
        -
work_dir: path  specifies working directory (default: .)
                defines a base path for all relative paths

targets:                allows to define targets, they are used only 
    name:source_path    when this file is specified as -f switch

If modules.yaml is missing, then all *.cpp files in current directory are scanned for
modules. All subdirectories used for mapping, wher name of directory is used as prefix

Special usage:
==============

cairn --scan <file.cpp> <compiler> <flags>  - run scanner for this file, 
                                              output result to stdout     
                                              (for testing)

cairn --preproc <file.cpp> <compiler> <flags> - run internal preprocessor
                                                output result to stdout 
                                                (for testing)



)help";

bool parse_cmdline(AppSettings &settings, CliReader<ArgumentString::value_type> &cli) {
    ArgumentString file;
    auto curdir = std::filesystem::current_path();
    while(true) {
        auto p = cli.next();
        if (p.is_end) return false;
        if (p.is_text) {
            auto t = std::basic_string_view(p.text);
            auto n = t.find(static_cast<ArgumentString::value_type>('='));
            if (n == t.npos) {
                cli.put_back();
                break;
            }
            auto target = t.substr(0,n);
            auto path = t.substr(n+1);
            settings.targets.push_back({(curdir/target).lexically_normal(),(curdir/path).lexically_normal()});
            continue;
        }
        if (p.is_long_sw) {
            if (ArgumentStringView(p.long_sw) == ArgumentConstant("scan") ) {
                settings.scan_file = (curdir/cli.text()).lexically_normal();
                break;
            } else if (ArgumentStringView(p.long_sw) == ArgumentConstant("preproc")) {
                settings.preproc_file = (curdir/cli.text()).lexically_normal();
                break;
            } else if (ArgumentStringView(p.long_sw) == ArgumentConstant("list")) {
                settings.list = true;                
                continue;
            } else {
                return false;
            }
        }

        switch (p.short_sw) {
            default: return false;
            case 'j': settings.threads = cli.number();break;
            case 'p': settings.compiler_type = cli.text();break;
            case 'c': settings.compile_commands_json = (curdir/cli.text()).lexically_normal();break;
            case 'f': settings.env_file_json = (curdir/cli.text()).lexically_normal();break;
            case 'b': settings.working_directory_path = (curdir/cli.text()).lexically_normal();break;
            case 'r': settings.recompile = true;break;
            case 'R': settings.drop_database = true;break;
            case 'C': settings.mode = AppSettings::compile_only;break;
            case 'L': settings.mode = AppSettings::link_only;break;            
            case 's': Log::set_level(Log::Level::error);break;
            case 'd': Log::set_level(Log::Level::debug);break;            
            case 'h': settings.show_help = true;break;
            case 'k': settings.keep_going = true;break;
            case 'l': settings.list = true;break;
            case 'M': settings.generate_makefile = (curdir/cli.text()).lexically_normal();break;
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

template<typename  Fn>
class ConstText : public std::string_view{
    static constexpr auto this_fn = Fn();    
    static constexpr auto need_space = this_fn(nullptr);
public:
    constexpr ConstText(Fn fn): std::string_view(text,need_space) {
        auto s =fn(text);
        text[s] = 0;
    }
protected:
    char text[need_space+1]={};
};

constexpr auto help_text = ConstText([](char *s)->std::size_t{
    std::size_t need_space = appname.size()+APP_VERSION.size()+licence.size()+helptext.size()+5;
    if (s != nullptr) {
        s = std::copy(appname.begin(), appname.end(), s);
        *s++ = ' ';
        s = std::copy(APP_VERSION.begin(), APP_VERSION.end(), s);
        *s++ = ' ';
        s = std::copy(licence.begin(), licence.end(), s);
        *s++ = '\n';
        *s++ = '\n';
        s = std::copy(helptext.begin(), helptext.end(), s);
        *s++ = '\n';
    }
    return need_space;
});


std::string_view get_help() {
    return help_text;
}