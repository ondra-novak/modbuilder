#include "temp_file.hpp"
#include <cstdio>
#include <filesystem>
#include <unistd.h>


OutputTempFile::~OutputTempFile() {
    reset();
}

void OutputTempFile::reset() {
    if (_f.is_open()) _f.close();
    if (!_commited_path.empty()) {
        std::filesystem::remove(_commited_path);
        _commited_path = std::filesystem::path();
    }
}

std::filesystem::path OutputTempFile::commit() {
    if (_f.is_open()) _f.close();
    return _commited_path;
}

std::ostream &OutputTempFile::create() {
    reset();

    int pid = ::getpid();

    char buffer[100];
    snprintf(buffer,sizeof(buffer), "cmdlst_%d_%p",pid,static_cast<void *>(this));
    auto tmp_dir = std::filesystem::temp_directory_path();
    _commited_path =  tmp_dir/buffer;
    _f.open(_commited_path, std::ios::out|std::ios::trunc);
    return _f;
}