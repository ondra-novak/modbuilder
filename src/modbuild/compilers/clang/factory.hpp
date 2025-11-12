#include "../../abstract_compiler.hpp"
#include <memory>

std::unique_ptr<AbstractCompiler> create_compiler_clang(AbstractCompiler::Config);