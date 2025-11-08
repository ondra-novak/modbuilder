#pragma once
#include "abstract_compiler.hpp"
#include "module_type.hpp"
#include <string>
#include <vector>
#include <filesystem>

class SourceScanner {
public:

    explicit SourceScanner(AbstractCompiler &compiler): _compiler(compiler) {}


    struct Info {
        std::string name;     //logical name of this module  (FQN for partition)
        ModuleType type = ModuleType::source;
        std::vector<std::string> required;  //list of logical names of required modules (partitions are FQN)
        std::vector<std::string> exported; //list of logical names of exported modules (must be also included as required)
        std::vector<std::string> include_q; //list of includes 
        std::vector<std::string> include_a; //list of angled includes
    };

    Info scan_file(const std::filesystem::path &path);

    static Info scan_string(std::string_view text);

protected:

    static Info scan_string_2(std::string_view text);

    AbstractCompiler &_compiler;
};