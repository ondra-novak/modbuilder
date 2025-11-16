add_library(compiler_gcc OBJECT compiler_gcc.cpp)

target_include_directories(compiler_gcc PRIVATE BEFORE ../../)