#pragma once

#include <filesystem> 


struct CompileTarget {
    std::filesystem::path target;
    std::filesystem::path source;
};