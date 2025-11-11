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
        "=========\n";
    for (auto &c: program_args) {

        if (!c.short_switch && c.long_switch.empty()) {
            std::cout << c.name;
            for (int i = c.name.size()/8; i < 3; ++i) std::cout<<'\t';
        }
        else {
            if (c.short_switch) std::cout << "-" << c.short_switch;
            std::cout << '\t';
            if (!c.long_switch.empty()) {
                std::cout << "--" << c.long_switch;
                if (c.long_switch.size()<6) std::cout << '\t';
            } else {
                std::cout << '\t';
            }
            std::cout << '\t';
        }
        std::cout << c.help << std::endl;
    }
    std::cout << "<compiler_path>\tPath to compiler (for example /usr/bin/g++)\n" 
                 "\n"
                 "Compiler/Linker switches"
                 "========================"
                 "All following arguments are directly passed to the compiler\n"
                 "However there are special arguments with following meaning\n"
                 "\n"
                 "--compile:\tFollowing arguments are passed only during a compile phase\n" 
                 "--link:   \tFollowing arguments are passed only a link phase\n"                  
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
    if (!rd) {
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