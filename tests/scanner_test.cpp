#include "../src/modbuild/scanner.hpp"
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
        case ModuleType::partition: typestr = "Partition";break;
        default:break;
    }
    

    std::cout << "Module name: "<< nfo.name << std::endl;
    std::cout << "Type: " << typestr << std::endl;
    std::cout << "Requires:" ;   print_list(nfo.required);
    std::cout << "Re-exports:"; print_list(nfo.exported);
    std::cout << "Headers quoted:"; print_list(nfo.include_q);
    std::cout << "Headers angled:"; print_list(nfo.include_a);
    std::cout << "---------" << std::endl;
}

constexpr std::string_view export_module = R"cpp(

module;

export module A;

export void foo();

export namespace {
    int bar()  {
        return (1+2)*3;
    }
}

import B;

class Foo {
public:
    Foo(std::unique_ptr<int> bar);
    ~Foo();
    std::string_view module_name() {return "export import C;";}
protected:
    std::unique_ptr<int> _ptr;
};

)cpp";


void test_scanner(std::string_view name, std::string_view str) {
    auto info = SourceScanner::scan_string(str);
    std::cout << name << ":" << std::endl;
    print_info(info);
}

constexpr std::string_view export_module_partitions = R"cpp(


export module A;
import Temp;
import :first;
import :second;
import Temp;
export import Test.A.B

)cpp";

constexpr std::string_view export_module_partitions2 = R"cpp(


export module A:first;
import B;
import :second;

export namespace neco;

)cpp";

constexpr std::string_view import_headers = R"cpp(

import <vector>;
import <string_view>;
import "cesta/soubor.hpp";
import module_name;

constexpr std::string_view mod = "import nothing;"

constexpr std::string_view long_str = R"txt(
    import nothing2;
    weiqoweq 
    )   )
    "weqwe"dweoieqeq
)txt";

import after_string;

)cpp";

constexpr std::string_view implementation_file = R"cpp(

module;
constexpr std::string_view non_module="non-module content";
module A;

import B;
import C.neco;


)cpp";

int main() {
    test_scanner("test1.cpp", export_module);
    test_scanner("test1_parts.cpp", export_module_partitions);
    test_scanner("test1_first_part.cpp", export_module_partitions2);
    test_scanner("header_import.cpp", import_headers);
    test_scanner("implementation.cpp", implementation_file);
}
