#pragma once
#include <streambuf>
#include <vector>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// Jednoduchý, přenositelný streambuf pro file descriptor
class fd_streambuf : public std::streambuf {
    static constexpr std::size_t buf_size = 4096;

    int fd_;
    bool owner_;
    std::vector<char> in_buffer_;
    std::vector<char> out_buffer_;

public:
    explicit fd_streambuf(int fd, bool take_ownership = false)
        : fd_(fd), owner_(take_ownership),
          in_buffer_(buf_size), out_buffer_(buf_size)
    {
        // input buffer empty
        setg(in_buffer_.data(), in_buffer_.data(), in_buffer_.data());
        // output buffer empty
        setp(out_buffer_.data(), out_buffer_.data() + out_buffer_.size());
    }

    ~fd_streambuf() override {
        sync();
        if (owner_ && fd_ >= 0)
            ::close(fd_);
    }

protected:
    // === čtení (input) ===
    int_type underflow() override {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());

        ssize_t n = ::read(fd_, in_buffer_.data(), in_buffer_.size());
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
    int flush_buffer() {
        ptrdiff_t n = pptr() - pbase();
        char* data = out_buffer_.data();
        char* end = data + n;
        ssize_t written = 0;

        while (data < end) {
            ssize_t res = ::write(fd_, data, end - data);
            if (res < 0) {
                if (errno == EINTR) continue;
                return -1;
            }
            data += res;
            written += res;
        }

        pbump(-n);
        return written;
    }
};
