#include "compiler_msvc.hpp"
#include "factory.hpp"

#include <stdexcept>

std::unique_ptr<AbstractCompiler> create_compiler_msvc( AbstractCompiler::Config) {
    throw std::runtime_error("Currently unsupported");
    return {};
}