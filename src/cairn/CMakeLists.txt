add_subdirectory(compilers/msvc)
add_subdirectory(compilers/clang)
add_subdirectory(compilers/gcc)



add_library(core OBJECT)
add_executable(cairn)

target_sources(core PRIVATE
        module_resolver.cpp
        module_database.cpp
        scanner.cpp
        abstract_compiler.cpp
        compile_commands_supp.cpp
        cli.cpp
        preprocess.cpp
        utils/log.cpp
        utils/process.cpp
        utils/temp_file.cpp
        utils/env.cpp
)   


target_sources(cairn PRIVATE main.cpp)
target_link_libraries(cairn PRIVATE 
       $<TARGET_OBJECTS:compiler_msvc>
       $<TARGET_OBJECTS:compiler_clang>
       $<TARGET_OBJECTS:compiler_gcc>
       $<TARGET_OBJECTS:core>
)


