module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

export module cairn.utils.process:win;

import cairn.utils.log;
import cairn.utils.fd_streambuf;
import cairn.utils.arguments;
import cairn.utils.env;

import <optional>;
import <filesystem>;
import <span>;


export class Process {
public:
    const HANDLE pid = NULL;
    std::optional<fd_streambuf> child_stdin_buf;
    std::optional<fd_streambuf> child_stdout_buf;
    std::optional<fd_streambuf> child_stderr_buf;
    std::optional<std::ostream> stdin_stream;
    std::optional<std::istream> stdout_stream;
    std::optional<std::istream> stderr_stream;

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    ~Process();

    int waitpid_status();
    void kill_child(int sig);
    void kill_child();

    enum StreamFlags {
        no_streams = 0,
        input = 1,
        output = 2,
        input_output =3,
        error = 4,
        input_error = 5,
        output_error = 6,
        input_output_error =7,
    };

    friend StreamFlags operator|(StreamFlags a, StreamFlags b) {
        return static_cast<StreamFlags>(static_cast<int>(a) | static_cast<int>(b));
    }
    friend int operator&(StreamFlags a, StreamFlags b) {
        return static_cast<int>(a) & static_cast<int>(b);
    }

    static Process spawn(const std::filesystem::path & path,
                         const std::filesystem::path & workdir,
                         const std::span<const ArgumentString>& args,
                         StreamFlags flags, std::optional<SystemEnvironment> env = std::nullopt);

protected:
    Process(HANDLE pid, HANDLE fdstdin, HANDLE fdstdout, HANDLE fdstderr);

};



Process::Process(HANDLE pid, HANDLE fdstdin, HANDLE fdstdout, HANDLE fdstderr)
    :pid(pid) {
        if (fdstdin) {
            child_stdin_buf.emplace(fdstdin, true);
            stdin_stream.emplace(&child_stdin_buf.value());
        }
        if (fdstdout) {
            child_stdout_buf.emplace(fdstdout, true);
            stdout_stream.emplace(&child_stdout_buf.value());
        }
        if (fdstderr) {
            child_stderr_buf.emplace(fdstderr, true);
            stderr_stream.emplace(&child_stderr_buf.value());
        }
}

int Process::waitpid_status() {
    if (pid == NULL) return -1;
    DWORD status;
    DWORD r = WaitForSingleObject(pid, INFINITE);
    if (r != WAIT_OBJECT_0) {
        throw std::runtime_error("WaitForSingleObject failed");
    }
    if (!GetExitCodeProcess(pid, &status)) {
        throw std::runtime_error("GetExitCodeProcess failed");
    }
    return static_cast<int>(status);    
}

void Process::kill_child(int sig) {
    if (pid != NULL) {
        TerminateProcess(pid, static_cast<UINT>(sig));
    }
}
void Process::kill_child() {
    kill_child(1);
}   

class FD {
public:
    FD (HANDLE fd = INVALID_HANDLE_VALUE):_fd(fd) {};
    FD(FD &&other):_fd(other._fd) {other._fd = INVALID_HANDLE_VALUE;}
    FD &operator=(FD &&other){
        if (this != &other) {
            close();
            _fd = other._fd;
            other._fd = INVALID_HANDLE_VALUE;            
        }
        return *this;
    }
    operator HANDLE() const {return _fd;}
    void close() {
        if (_fd != INVALID_HANDLE_VALUE) ::CloseHandle(_fd);
    }
    ~FD() {close();}
    HANDLE release() {
        HANDLE r =_fd;
        _fd = INVALID_HANDLE_VALUE ;
        return r;
    }

    
protected:
    HANDLE _fd = INVALID_HANDLE_VALUE;
};

struct Pipe {
    FD read;
    FD write;
    static Pipe create() {
        HANDLE read, write;
        BOOL res = CreatePipe(&read, &write, NULL, 0);
        if (!res) throw std::system_error(GetLastError(), std::system_category(), "Unable to open pipe");
        return {FD(read), FD(write)};
    }
};

static void make_inherit(HANDLE handle) {
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
        throw std::system_error(GetLastError(), std::system_category(), "Unable to set handle inheritance");
    }
}


// Funkce escapuje jednotlivé argumenty podle pravidel CreateProcess
std::wstring quote_argument(const std::wstring& arg) {
    // Pokud argument neobsahuje mezery ani uvozovky, vrací se přímo
    if (arg.empty() || arg.find_first_of(L" \t\"") != std::wstring::npos) {
        std::wstring escaped = L"\"";
        size_t backslashes = 0;
        for (wchar_t c : arg) {
            if (c == L'\\') {
                backslashes++;
            } else if (c == L'"') {
                escaped.append(backslashes * 2 + 1, L'\\'); // escape \ před uvozovkou
                escaped.push_back(L'"');
                backslashes = 0;
            } else {
                escaped.append(backslashes, L'\\');
                backslashes = 0;
                escaped.push_back(c);
            }
        }
        escaped.append(backslashes, L'\\'); // závěrečné \ před koncem uvozovky
        escaped.push_back(L'"');
        return escaped;
    } else {
        return arg;
    }
}

// Sestaví command line pro CreateProcess z programu a argumentů
std::wstring build_command_line(const std::filesystem::path& program,
                                std::span<const ArgumentString> args) {
    std::wstringstream ss;
    ss << quote_argument(program.wstring());
    for (const auto& arg : args) {
        ss << L" " << quote_argument(arg);
    }
    return ss.str();

}

Process Process::spawn(const std::filesystem::path & path,
                         const std::filesystem::path & workdir,
                         const std::span<const ArgumentString>& args,
                         StreamFlags flags, std::optional<SystemEnvironment> env) {
    std::optional<Pipe> pstdin;
    std::optional<Pipe> pstdout;
    std::optional<Pipe> pstderr;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.cb = sizeof(si);
    if (flags & input) {
        pstdin = Pipe::create();
        si.hStdInput = pstdin->read;
        make_inherit(si.hStdInput);
    } else {
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    if (flags & output) {
        pstdout = Pipe::create();
        si.hStdOutput = pstdout->write;
        make_inherit(si.hStdOutput);        
    } else {
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    if (flags & error) {
        pstderr = Pipe::create();
        si.hStdError = pstderr->write;
        make_inherit(si.hStdError);        
    } else {
        si.hStdError =  GetStdHandle(STD_ERROR_HANDLE);
    }

    std::wstring command_line = build_command_line(path, args);
    std::wstring workdir_w = workdir.wstring();
    std::wstring env_w;
    LPVOID env_ptr = nullptr;
    if (env.has_value()) {
        env_w = env->to_windows_format();
        env_ptr = env_w.data();
    }   

    Log::debug("execute: {}", [&]{
        auto sz = WideCharToMultiByte(CP_UTF8,0, command_line.c_str(), static_cast<int>(command_line.size()),NULL,0,0,FALSE);
        std::string tmp(sz,0);
        WideCharToMultiByte(CP_UTF8,0, command_line.c_str(), static_cast<int>(command_line.size()),tmp.data(),static_cast<int>(tmp.size()),0,FALSE);
        return tmp;
    });

    BOOL res = CreateProcessW(
        NULL,
        command_line.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_UNICODE_ENVIRONMENT,
        env_ptr,
        workdir_w.empty() ? NULL : workdir_w.data(),
        &si,
        &pi
    );
    if (!res) {
        throw std::system_error(GetLastError(), std::system_category(), "Unable to spawn process");
    }
    // Close unneeded handles
    CloseHandle(pi.hThread);

    return Process (pi.hProcess,
        pstdin?pstdin->write.release():INVALID_HANDLE_VALUE,
        pstdout?pstdout->read.release():INVALID_HANDLE_VALUE,
        pstderr?pstderr->read.release():INVALID_HANDLE_VALUE);
}
   

Process::~Process() {
    if (pid != NULL) {
        CloseHandle(pid);
    }
}