add_executable(scanner_test.exe)

target_sources(scanner_test.exe PRIVATE
        scanner_test.cpp
        ../src/modbuild/scanner.cpp
)   


