#include "../src/modbuild/scanner.hpp"
#include "../src/modbuild/compilers/gcc/compiler_gcc.hpp"
#include <filesystem>
#include <iostream>

void print_info(SourceScanner::Info nfo) {
    
    auto print_list = [](const auto &lst) {
        char sep = ' ';
        for (const auto &x: lst) {
            std::cout << sep << " " <<x;
            sep = ',';
        }
        std::cout << std::endl;
    };

    std::string_view typestr;
    switch (nfo.type) {
        case ModuleType::source: typestr = "Source file";break;
        case ModuleType::interface: typestr = "Interface";break;
        case ModuleType::implementation: typestr = "Implementation";break;
        default:break;
    }
    

    std::cout << "Module name: "<< nfo.name << std::endl;
    std::cout << "Type: " << typestr << std::endl;
    std::cout << "Requires:" ;   print_list(nfo.required);
    std::cout << "Re-exports:"; print_list(nfo.exported);
    std::cout << "Headers quoted:"; print_list(nfo.user_headers);
    std::cout << "Headers angled:"; print_list(nfo.system_headers);
    std::cout << "---------" << std::endl;
}

int main() {
    auto path = std::filesystem::path("module_example/modA/modA_code.cpp");
    std::vector<ArgumentString> args = {
        inline_arg("g++"),
        inline_arg("-Imodule_example/includes"),
    };
    auto compiler = CompilerGcc::create(args);

    SourceScanner scanner(*compiler);
    std::cout << path << std::endl;
    print_info(scanner.scan_file(path));

}