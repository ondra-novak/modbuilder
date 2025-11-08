#pragma once
#include "../../abstract_compiler.hpp"
#include "../../utils/arguments.hpp"
#include <vector>

class CompilerMSVC: public AbstractCompiler {
public:


protected:
    std::filesystem::path _program_path;    //path to cl.exe
    std::vector<ArgumentString> _compile_options;   //common compile options
    std::vector<ArgumentString> _link_options;      //common link options

    
};