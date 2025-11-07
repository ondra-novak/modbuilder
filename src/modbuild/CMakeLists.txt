add_subdirectory(compilers/msvc)

add_executable(modbuild.exe)

target_sources(modbuild.exe PRIVATE
        main.cpp
        module_resolver.cpp
        module_database.cpp
        scanner.cpp
)   

target_link_libraries(modbuild.exe PRIVATE
       $<TARGET_OBJECTS:compiler_msvc>
)

