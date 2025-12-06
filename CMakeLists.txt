cmake_minimum_required(VERSION 3.28)
project(cairn)

include_directories(${CMAKE_BINARY_DIR})
find_package(Git QUIET)
if(GIT_FOUND)

    # Generate version from git
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    configure_file(
        ${CMAKE_SOURCE_DIR}/version.cpp.in
        ${CMAKE_SOURCE_DIR}/src/cairn/version.cpp
    )

else()
    message(WARNING "Git not found. Submodules may not be initialized.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    set(COMPILE_FLAGS /W4 /EHsc /std:c++20)
else()
    set(COMPILE_FLAGS -Wall -Wextra -Wpedantic -std=c++20)
endif()

separate_arguments(FLAGS_DEBUG NATIVE_COMMAND ${CMAKE_CXX_FLAGS_DEBUG})
separate_arguments(FLAGS_RELEASE NATIVE_COMMAND ${CMAKE_CXX_FLAGS_RELEASE})
separate_arguments(FLAGS_RELWITHDEBINFO NATIVE_COMMAND ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
separate_arguments(FLAGS_MINSIZEREL NATIVE_COMMAND ${CMAKE_CXX_FLAGS_MINSIZEREL})


add_custom_target(cairn ALL
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/.install/cairn 
        -j0 -B${CMAKE_BINARY_DIR}
        -c${CMAKE_BINARY_DIR}/compile_commands.json 
        -f cairn.yaml
#        ${CMAKE_CXX_COMPILER} ${COMPILE_FLAGS} ${FLAGS_RELEASE}
    ${CMAKE_CXX_COMPILER} ${COMPILE_FLAGS} /Od /Zi 
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND_EXPAND_LISTS   )
