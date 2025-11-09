#include "../src/modbuild/module_database.hpp"
#include "../src/modbuild/compilers/clang/compiler_clang.hpp"
#include "../src/modbuild/utils/log.hpp"
#include <iostream>
#include <json/serializer.h>

int main() {

    Log::set_level(Log::Level::debug);
    auto path = std::filesystem::path("module_example/basic/test.cpp");
    path = std::filesystem::canonical(path);
    std::vector<ArgumentString> args = {
        inline_arg("clang++"),
        inline_arg("-Imodule_example/includes"),
    };
    auto compiler = CompilerClang::create(args, std::filesystem::current_path());

    ModuleDatabase db;
    auto unsat = db.rescan_file(path, path.parent_path(), *compiler, true);

    for (auto &x: unsat) std::cout << x.name << std::endl;
    
    std::cout << db.export_db().to_json() << std::endl;

    auto plan = db.create_compile_plan(path);
    for (auto &x: plan) {
        std::cerr << x.sourceInfo->source_file << "(" << static_cast<int>(x.sourceInfo->type) << ") :";
        for (auto &y: x.references) {
            std::cerr << " " << y->source_file;
        }
        std::cerr << std::endl;
    }

}
