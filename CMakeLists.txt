cmake_minimum_required(VERSION 3.20)
project(cairn)

include_directories(${CMAKE_BINARY_DIR})
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL 0)
        message(WARNING "Failed to update git submodules.")
    endif()

    # Generate version from git
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/version.hpp.in
        ${CMAKE_SOURCE_DIR}/src/cairn/version.hpp
    )

else()
    message(WARNING "Git not found. Submodules may not be initialized.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/W4 /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(BEFORE src/json/src)

add_subdirectory(src/cairn)
# add_subdirectory(tests)
