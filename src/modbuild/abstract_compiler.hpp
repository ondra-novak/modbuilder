#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <span>

class AbstractCompiler {
public:

    struct Depends {
        std::string name;     //logical name of this module  (FQN for partition)
        bool is_interface;    //true, if module is interface (generates BMI), false if it is implementation (must be also in required)
        std::vector<std::string> required;  //list of logical names of required modules (partitions are FQN)
        std::vector<std::string> exported; //list of logical names of exported modules (must be also included as required)
    };

    struct CompileResult {
        std::filesystem::path interface;
        std::filesystem::path object;
    };

    struct ModuleMapping {
        std::string logical_name;
        std::filesystem::path interface;
    };


    virtual ~AbstractCompiler() = default;


    virtual int scan(std::filesystem::path file, Depends &depends) const = 0;
    
    virtual int compile(std::filesystem::path source, 
        std::span<const ModuleMapping> modules,
        CompileResult &result) const = 0;
    
    virtual int link(std::filesystem::path binary, 
        std::span<const std::filesystem::path> objects) const = 0;

    virtual std::string preproces(const std::filesystem::path &file) const = 0;
    
};
