// file: process_spawn.hpp

#include "process.hpp"
#include <cstddef>
#include <numeric>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <ext/stdio_filebuf.h> // GNU extension
#include <signal.h>
#include <cpp.20/FixSizeVector.hpp>

extern char **environ;


void Process::close_streams() {
    if (stdin_stream) {
        stdin_stream->flush();
        stdin_stream.reset();
    }
    stdout_stream.reset();
    child_stdin_buf.reset();
    child_stdout_buf.reset();
}

int Process::waitpid_status() {
    if (pid <= 0) return -1;
    int status;
    pid_t r;
    do {
        r = ::waitpid(pid, &status, 0);
    } while (r == -1 && errno == EINTR);
    if (r == -1) {
        throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
    }
    return status;
}

void Process::kill_child(int sig) {
    if (pid > 0) ::kill(pid, sig);
}
void Process::kill_child() {
    kill_child(SIGTERM);
}

Process Process::spawn(const std::filesystem::path & path,
                         const std::vector<ArgumentString>& args) {
    int in_pipe[2];   // parent writes -> child stdin
    int out_pipe[2];  // child writes -> parent stdout

    if (::pipe(in_pipe) == -1 || ::pipe(out_pipe) == -1) {
        throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // Redirect stdin/stdout
    posix_spawn_file_actions_adddup2(&actions, in_pipe[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, out_pipe[1], STDOUT_FILENO);

    // Close unused ends in child
    posix_spawn_file_actions_addclose(&actions, in_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, out_pipe[0]);

    std::string pathstr = path.string();

    std::size_t reqspace = pathstr.length()+1+std::accumulate(args.begin(), args.end(), std::size_t(0), 
        [](std::size_t a, const ArgumentString &s) {
            return a + s.length()+1;
        });        
                         

    FixSizeVector<char> arg_buffer(reqspace);
    FixSizeVector<char *> pointers(args.size()+2);  //args + arg0 + NULL;

    {
        char *wrt = arg_buffer.data();
        auto ptr_iter = pointers.begin();
        *ptr_iter++ = wrt;
        wrt = std::copy(pathstr.begin(), pathstr.end(), wrt);
        *wrt++ = 0;
        for (const auto &a: args) {
            *ptr_iter++ = wrt;
            wrt = std::copy(a.begin(), a.end(), wrt);
            *wrt++ = 0;
        }
        *ptr_iter = nullptr;
    }
    
    pid_t child_pid;
    int rc = posix_spawnp(&child_pid,
                            pathstr.c_str(),
                            &actions,
                            nullptr,
                            pointers.data(),
                            environ);

    posix_spawn_file_actions_destroy(&actions);
    // Parent closes ends not used
    ::close(in_pipe[0]);
    ::close(out_pipe[1]);

    if (rc != 0) {
        ::close(in_pipe[1]);
        ::close(out_pipe[0]);
        throw std::runtime_error(std::string("posix_spawn failed: ") + std::strerror(rc));
    }

    Process p;
    p.pid = child_pid;

    // Wrap pipes into streams
    p.child_stdout_buf = std::make_unique<fd_streambuf>(out_pipe[0], true);
    p.stdout_stream = std::make_unique<std::istream>(p.child_stdout_buf.get());

    p.child_stdin_buf = std::make_unique<fd_streambuf>(in_pipe[1],true);
    p.stdin_stream = std::make_unique<std::ostream>(p.child_stdin_buf.get());

    return p;
}

