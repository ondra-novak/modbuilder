#include "../src/modbuild/module_database.hpp"
#include "../src/modbuild/compilers/clang/factory.hpp"
#include "../src/modbuild/utils/log.hpp"
#include "../src/modbuild/builder.hpp"
#include <iostream>
#include <json/serializer.h>

int main() {

    Log::set_level(Log::Level::debug);
    auto path = std::filesystem::path("module_example/basic/test.cpp");
    path = std::filesystem::canonical(path);
    
    auto compiler = create_compiler_clang({
        find_in_path("clang++"), {},{}, ".build"
    });

    ModuleDatabase db;
    auto unsat = db.rescan_file_discovery(nullptr, path, *compiler);

    for (auto &x: unsat) std::cout << x.name << std::endl;
    
    std::cout << db.export_db().to_json() << std::endl;
    auto plan = db.create_compile_plan(path);

    Builder bld(1, *compiler);
    CompileCommandsTable ctable;
    bld.generate_compile_commands(ctable, plan);
    std::cout << ctable.export_db().to_json() << std::endl;

}
