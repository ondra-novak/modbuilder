#pragma once
#include <fstream>
#include <filesystem>

class OutputTempFile {
public:

    std::ostream &create();

    std::filesystem::path commit();

    void reset();

    OutputTempFile() = default;
    ~OutputTempFile();

protected:

    std::ofstream _f;
    std::filesystem::path _commited_path;



};