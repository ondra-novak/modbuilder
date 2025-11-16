add_library(compiler_clang OBJECT compiler_clang.cpp)

target_include_directories(compiler_clang PRIVATE BEFORE ../../)