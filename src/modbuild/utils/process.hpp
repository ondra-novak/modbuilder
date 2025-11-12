#pragma once

#include "fd_streambuf.hpp"
#include <filesystem>
#include <memory>

#include <iostream>
#include <vector>
#include "arguments.hpp"
#include <ext/stdio_filebuf.h>

class Process {
public:
    pid_t pid = -1;
    std::unique_ptr<fd_streambuf> child_stdout_buf;
    std::unique_ptr<fd_streambuf> child_stdin_buf;
    std::unique_ptr<std::istream> stdout_stream;
    std::unique_ptr<std::ostream> stdin_stream;

    Process() = default;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&) noexcept = default;
    Process& operator=(Process&&) noexcept = default;

    ~Process() {
        close_streams();
    }
    void close_streams();
    int waitpid_status();
    void kill_child(int sig);
    void kill_child();

    static Process spawn(const std::filesystem::path & path,
                         const std::filesystem::path & workdir,
                         const std::span<const ArgumentString>& args,
                         bool no_streams);


};

