#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <span>
#include "utils/arguments.hpp"
#include "utils/which.hpp"

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

    struct Config {
        std::filesystem::path program_path;
        std::vector<ArgumentString> compile_options;
        std::vector<ArgumentString> link_options;
    };

    virtual ~AbstractCompiler() = default;

    
    
    virtual int compile(std::filesystem::path source, 
        std::span<const ModuleMapping> modules,
        CompileResult &result) const = 0;
    
    virtual int link(std::filesystem::path binary, 
        std::span<const std::filesystem::path> objects) const = 0;

    virtual std::string preproces(const std::filesystem::path &file) const = 0;
    

    static constexpr auto compile_flag = ArgumentConstant("--compile:");
    static constexpr auto link_flag = ArgumentConstant("--link:");

    enum class State {
        common, compile, link
    };

    static Config parse_commandline(const std::span<const ArgumentString> &args) {
        Config out;
        State st = State::common;
        if (args.empty()) return out;

        auto found = find_in_path(args[0]);
        if (found.has_value()) out.program_path = std::move(found.value());
        else out.program_path = args[0];

        auto params = args.subspan(1);

        for (auto &a: params) {
            if (a == compile_flag) st = State::compile;
            else if (a == link_flag) st = State::link;
            else { 
                switch (st) {
                    case State::common: out.compile_options.push_back(a);
                                        out.link_options.push_back(a);break;
                    case State::compile: out.compile_options.push_back(a);break;
                    case State::link: out.link_options.push_back(a);break;
                }
            }            
        }
        return out;

    }

};
