#add_subdirectory(compilers/msvc)
add_subdirectory(compilers/clang)
#add_subdirectory(compilers/gcc)



add_library(modbuildlb)
add_executable(modbuild)

target_sources(modbuildlb PRIVATE
        module_resolver.cpp
        module_database.cpp
        scanner.cpp
        discovery.cpp
        utils/log.cpp
        utils/process.cpp
        utils/temp_file.cpp
)   

target_link_libraries(modbuildlb PRIVATE
#       $<TARGET_OBJECTS:compiler_msvc>
       $<TARGET_OBJECTS:compiler_clang>
#       $<TARGET_OBJECTS:compiler_gcc>
)

target_sources(modbuild PRIVATE main.cpp)
target_link_libraries(modbuild PRIVATE modbuildlb)


