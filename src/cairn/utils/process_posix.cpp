// file: process_spawn.hpp

#include "process.hpp"
#include "log.hpp"
#include <cstddef>
#include <filesystem>
#include <numeric>
#include <spawn.h>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <signal.h>


extern char **environ;

Process::Process(int pid, int fdstdin, int fdstdout, int fdstderr)
    :pid(pid) {
        if (fdstdin>=0) {
            child_stdin_buf.emplace(fdstdin, true);
            stdin_stream.emplace(&child_stdin_buf.value());
        }
        if (fdstdout>=0) {
            child_stdout_buf.emplace(fdstdout, true);
            stdout_stream.emplace(&child_stdout_buf.value());
        }
        if (fdstderr>=0) {
            child_stderr_buf.emplace(fdstderr, true);
            stderr_stream.emplace(&child_stderr_buf.value());
        }
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

static constexpr std::string_view spawn_text("spawn: ");

static int posix_spawn_verbose (pid_t * __pid,
			const char * __path,
			const posix_spawn_file_actions_t * __file_actions,
			const posix_spawnattr_t * __attrp,
			char *const __argv[],
			char *const __envp[]) {

    if (Log::is_level_enabled(Log::Level::debug)) {
        auto &buff = Log::get_buffer(Log::Level::debug);
              
        buff.insert(buff.end(), spawn_text.begin(), spawn_text.end());
        auto c = __argv;
        while (*c) {
            buff.push_back(' ');
            auto strl = std::string_view(*c);
            buff.insert(buff.end(), strl.begin(), strl.end());            
            ++c;
        }      
        Log::send_buffer(Log::Level::debug, buff);
    }

    return posix_spawn(__pid,__path,__file_actions,__attrp, __argv,__envp);

}

class FD {
public:
    FD (int fd = -1):_fd(fd) {};
    FD(FD &&other):_fd(other._fd) {other._fd = -1;}
    FD &operator=(FD &&other){
        if (this != &other) {
            close();
            _fd = other._fd;
            other._fd = -1;            
        }
        return *this;
    }
    operator int() const {return _fd;}
    void close() {
        if (_fd != -1) ::close(_fd);
    }
    ~FD() {close();}
    int release() {
        int r =_fd;
        _fd = -1;
        return r;
    }

    
protected:
    int _fd = -1;
};

struct Pipe {
    FD read;
    FD write;
    static Pipe create() {
        int p[2];
        if (::pipe2(p, O_CLOEXEC) == -1) throw std::system_error(errno, std::system_category(), "Unable to open pipe");
        return {p[0],p[1]};
    }
};

    
Process Process::spawn(const std::filesystem::path &path, 
        const std::filesystem::path &workdir,
        const std::span<const ArgumentString> &args, 
        StreamFlags streams, const std::optional<SystemEnvironment> &env) {
            
    std::optional<Pipe> pstdin;
    std::optional<Pipe> pstdout;
    std::optional<Pipe> pstderr;

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    if (streams & input) {
        pstdin = Pipe::create();
        posix_spawn_file_actions_adddup2(&actions, pstdin->read, STDIN_FILENO);
        posix_spawn_file_actions_addclose(&actions, pstdin->read);
    }
    if (streams & output) {
        pstdout = Pipe::create();
        posix_spawn_file_actions_adddup2(&actions, pstdout->write, STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, pstdout->write);
    }
    if (streams & error) {
        pstderr = Pipe::create();
        posix_spawn_file_actions_adddup2(&actions, pstderr->write, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, pstderr->write);
    }


    auto cd = std::filesystem::current_path();
    auto cd_ret = std::unique_ptr<std::filesystem::path, decltype([](auto *x){
        std::filesystem::current_path(*x);
    })>(&cd);
    
    std::error_code ec;
    std::filesystem::current_path(workdir, ec);
    if (ec != std::error_code{}) {
        throw std::system_error(ec, workdir);
    }

    std::string pathstr = path.string();

    std::size_t reqspace = pathstr.length()+1+std::accumulate(args.begin(), args.end(), std::size_t(0), 
        [](std::size_t a, const ArgumentString &s) {
            return a + s.length()+1;
        });        
                         

    std::vector<char> arg_buffer(reqspace);
    std::vector<char *> pointers(args.size()+2);  //args + arg0 + NULL;

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

    char **pointers_env = environ;
    SystemEnvironment::Buffer env_buffer;
    if (env.has_value()) {
        pointers_env = reinterpret_cast<char **>(env->posix_format(env_buffer));
    }
    

    pid_t child_pid;
    int rc = posix_spawn_verbose(&child_pid,
                            pathstr.c_str(),
                            &actions,
                            nullptr,
                            pointers.data(),
                            pointers_env);

    posix_spawn_file_actions_destroy(&actions);

    if (rc != 0) {
        throw std::runtime_error(std::string("posix_spawn failed: ") + std::strerror(rc));
    }

    return Process (child_pid,
        pstdin?pstdin->write.release():-1,
        pstdout?pstdout->read.release():-1,
        pstderr?pstderr->read.release():-1    
    );
}
