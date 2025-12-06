module;



#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "errno.h"
#include "string.h"


export module cairn.utils.fd_streambuf;
import <streambuf>;
import <vector>;


// Jednoduchý, přenositelný streambuf pro file descriptor
export class fd_streambuf : public std::streambuf {
    static constexpr std::size_t buf_size = 4096;
public:

    #ifdef _WIN32
    using FD = HANDLE;
    static constexpr FD invalid_fd = INVALID_HANDLE_VALUE;
    #else
    using FD = int;
    static constexpr FD invalid_fd = -1;
    #endif
    

    explicit fd_streambuf(FD fd, bool take_ownership = false)
        : fd_(fd), owner_(take_ownership),
          in_buffer_(buf_size), out_buffer_(buf_size)
    {
        // input buffer empty
        setg(in_buffer_.data(), in_buffer_.data(), in_buffer_.data());
        // output buffer empty
        setp(out_buffer_.data(), out_buffer_.data() + out_buffer_.size());
    }

    fd_streambuf(fd_streambuf &&other)
        :fd_(other.fd_)
        ,owner_(other.owner_)
        ,in_buffer_(std::move(other.in_buffer_))
        ,out_buffer_(std::move(other.out_buffer_)) {
            other.owner_ = false;
        }

    fd_streambuf &operator=(fd_streambuf &&other) {
        if (this != &other) {
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
        }
        return *this;
    }

    ~fd_streambuf() override {
        sync();
        if (owner_ && fd_ != invalid_fd ) {
#ifdef _WIN32
            CloseHandle(fd_);
#else
            ::close(fd_);
#endif
        }
    }

protected:    

    // === čtení (input) ===
    int_type underflow() override {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());
#ifdef _WIN32
        DWORD n;
        if (!ReadFile(fd_, in_buffer_.data(), static_cast<DWORD>(in_buffer_.size()), &n, NULL)) n = 0;
#else
        ssize_t n = ::read(fd_, in_buffer_.data(), in_buffer_.size());
#endif
        if (n <= 0) {
            // EOF nebo chyba
            return traits_type::eof();
        }

        setg(in_buffer_.data(), in_buffer_.data(), in_buffer_.data() + n);
        return traits_type::to_int_type(*gptr());
    }

    // === zápis (output) ===
    int_type overflow(int_type ch) override {
        if (!traits_type::eq_int_type(ch, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }
        if (flush_buffer() == -1)
            return traits_type::eof();
        return traits_type::not_eof(ch);
    }

    int sync() override {
        return (flush_buffer() == -1) ? -1 : 0;
    }

private:
    FD fd_;
    bool owner_;
    std::vector<char> in_buffer_;
    std::vector<char> out_buffer_;

    int flush_buffer() {
        ptrdiff_t n = pptr() - pbase();
        char* data = out_buffer_.data();
        char* end = data + n;
        size_t written = 0;

        while (data < end) {
#ifdef _WIN32
            DWORD res;
            if (!WriteFile(fd_,data, static_cast<DWORD>(end - data), &res, NULL)) res = 0;
#else
            ssize_t res = ::write(fd_, data, end - data);
#endif
            if (res < 0) {
                if (errno == EINTR) continue;
                return -1;
            }
            data += res;
            written += res;
        }

        pbump(static_cast<int>(-n));
        return static_cast<int>(written);
    }
};
