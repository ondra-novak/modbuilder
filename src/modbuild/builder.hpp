#pragma once

#include "abstract_compiler.hpp"
#include "module_database.hpp"
#include "utils/thread_pool.hpp"
#include <future>


class Builder {
public:

    Builder(std::size_t threads, AbstractCompiler &compiler);

    std::future<bool> build(std::vector<ModuleDatabase::CompilePlan> plan, bool stop_on_error);


protected:

    ThreadPool _thrp;
    AbstractCompiler &_compiler;
};