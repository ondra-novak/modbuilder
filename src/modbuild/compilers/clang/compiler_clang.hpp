#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"

class CompilerClang : public AbstractCompiler {
public:

    virtual int compile(std::filesystem::path source, 
        std::span<const ModuleMapping> modules,
        CompileResult &result) const override;
    
    virtual int link(std::filesystem::path binary, 
        std::span<const std::filesystem::path> objects) const override;

    virtual std::string preproces(const std::filesystem::path &file) const override;

    CompilerClang(Config config):_config(config) {}


    static std::unique_ptr<AbstractCompiler> create(std::span<const ArgumentString> arguments);


    static constexpr auto preprocess_flag = ArgumentConstant("-E");

protected:
        Config _config;


    
};