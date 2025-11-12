#include "utils/arguments.hpp"
#include <vector>
#include "app.hpp"


static constexpr std::array<ArgumentDef<App>, 6> program_args ({
    {'h',"help",&App::arg_show_help,"", "show help"},
    {'j',"threads",&App::arg_set_threads,"", "specify count of threads for parallel build"},
    {'c',"compiler",&App::arg_set_compiler, "", "specify compiler type (gcc|clang|msvc)"},
    {'p',"project",&App::arg_set_project, "", "specify path to project configuration"},
    {0,"",&App::arg_set_compile_file, "<file.cpp>", "specify path to compiled file (mandatory)"},
    {0,"",&App::arg_set_compile_file, "<compiler>","specify compiler command or path (mandatory)"},
});


bool App::arg_show_help()  {
    std::cout << "Usage:\n\n" 
        "modbuild [...switches...] <file.cpp>  <compiler> [...compiler/linker..flags...]\n"
        "\n"
        "Switches:\n"
        "=========\n"
        "-jN          specify count of threads\n"
        "-p<type>  select compiler type: gcc|clang|msvc \n"
        "-c<path>  generate compile_commands.json (where)\n"
        "-s        output only errors (silent)\n"
        "-d        debug mode (output everyting)\n"
        "-f<file>  specify environment file (modules.json) for this build\n"
        "-b<dir>   specify build directory\n"
        "-C        compile only (don't run linker)\n"
        "-L        link only (requires compiled files in build directory)\n"
        "-W        specified file is modules.json to compile (all files)\n"
        "\n"
        "file.cpp  specifies path to file to compile. If -W is used, then\n"
        "          then it specifies pathname to modules-like json file\n"
        "          to compile\n"
        "\n"
        "compiler  path to compiler's binary. PATH is used to search binary\n"
        "\n"
        "Compiler arguments\n"
        "=================\n"
        "Any arguments here are copied to command-line when compiler is invoked\n"
        "However, there are switches that have special meaning (are not copied)\n"
        "Note these switches end with colon\n"
        "\n"
        "--compile:  following arguments are used only during compilation phase\n"
        "--link:     following arguments are used only during link phase\n"
        "--lib:      produces library (will not link), following argumenst are \n"
        "            used for librarian tool\n"
        "\n"
        "Example: gcc -DSPECIAL -I/usr/local/include --compile: -O2 -march=native --link: -o example -lthread\n";

    exit(0);
    return true;
        
}

template<typename T>
int tmain(int argc, T *argv[]) {

    App app;
    

    CliReader<T> cmdline(argc-1, argv+1);
    bool succ =process_arguments<App>(program_args, app, cmdline);
    if (!succ) return 1;
    if (app.source_file.empty()) {
        std::cerr << "No source file specified, type h for help";
        return 1;
    }
    if (!cmdline) {
        std::cerr << "Expected more arguments (compiler)";
        return 1;
    }
    
    //AbstractCompiler::parse_commandline()



    //todo entry point
    return 0;

}


#ifdef _WIN32
int wmain(int argc, wchar_t *argv[]) {
    return tmain(argc, argv);
}
#else
int main(int argc, char *argv[]) {
    return tmain<char8_t>(argc,reinterpret_cast<char8_t **>(argv));
}
#endif