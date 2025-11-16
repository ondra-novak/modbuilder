#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <filesystem>
#include <span>
#include <algorithm>
#include <iostream>


#ifdef _WIN32 
using ArgumentStringView = std::wstring_view;
using ArgumentString = std::wstring;
#else
using ArgumentStringView = std::u8string_view;
using ArgumentString = std::u8string;
#endif

template<int N>
inline ArgumentString inline_arg(const char (&str)[N]) {
    ArgumentString ret;
    ret.reserve(N);
    std::string_view s(str);
    for (char c: s) ret.push_back(c);
    return ret;
}

inline ArgumentString string_arg(std::string_view str) {
    ArgumentString ret;
    for (char c: str) ret.push_back(c);
    return ret;
}

inline ArgumentString path_arg(const std::filesystem::path &p) {
#ifdef _WIN32 
    return ArgumentString(p.native());
#else
    return ArgumentString(p.u8string());
#endif

}

template<std::size_t N> 
class ArgumentConstant: public ArgumentStringView {
public:
    constexpr ArgumentConstant(const char *str)
        :ArgumentStringView(text, N) {
            for (std::size_t i = 0; i < N; ++i) {
                text[i] = static_cast<ArgumentStringView::value_type>(str[i]);
            }
        }

protected:
    ArgumentStringView::value_type text[N] = {};
};

template<std::size_t N>
ArgumentConstant(const char (&str)[N]) -> ArgumentConstant<N-1>;


template<typename T>
struct CliReader {
    
    CliReader(int argc, T *const * argv): argc(argc), argv(argv) {}

    int argc;
    T *const *argv;
    const T *nx = 0;

    struct Item {
        bool is_short_sw: 1 = 0;
        bool is_long_sw: 1 = 0;
        bool is_text:1 = 0;
        bool is_end:1 = 0;
        union {
            T short_sw;
            const T *long_sw;
            const T *text;
        };
        operator bool () const {return !is_end;}
    };

    ///read next item and determine type
    Item next() {
        if (nx && *nx) {
            T c = *nx++;
            return Item{.is_short_sw = true, .short_sw = c};
        } 
        if (!argc) return Item{.is_end = true, .short_sw = 0};
        T *arg = *argv;
        --argc;
        ++argv;
        if (arg[0] == '-') {
            if (arg[1] == '-') {
                return Item {.is_long_sw = true, .long_sw = arg+2} ;
            } else {
                nx = arg+2;
                return Item {.is_short_sw = true, .short_sw = arg[1]};
            }
        } else {
            return Item{.is_text = true, .text = arg};
        }
    }

    ///read next item as plain text
    const T *text() {
        const T *out = nullptr;
        if (nx && *nx) {
            out = nx;
            nx = nullptr;
        } else if (argc) {
            out = *argv;
            --argc;
            ++argv;
        } 
        return out;
    }

    ///read next item as a number
    int number() {
        int n = 0;
        bool ng = false;
        const T *iter;
        if (nx && *nx) {
            iter = nx;
        } else if (argc) {
            nx = nullptr;
            iter = *argv;
            ++argv;
            --argc;
        }  else {
            return 0;
        }

        if (*iter == '-') {
            ng = true;
            ++iter;
        }
        while (*iter >= '0' && *iter <= '9') {
            unsigned char c = static_cast<unsigned char>(*iter);
            n = n * 10 + (c - '0');
            ++iter;
        }
        if (nx) nx = iter;
        if (ng) n = -n;
        return n;
    }
    void put_back() {
        ++argc;
        --argv;
    }

    operator bool() const {
        return argc != 0 || (nx && *nx);
    }
};

template<typename AppendableContainer>
requires (requires(AppendableContainer c, ArgumentStringView v){{c.emplace_back(v)};})
void append_arguments(AppendableContainer &cont, 
    std::initializer_list<std::string_view> argtemplate, 
    std::initializer_list<ArgumentStringView> values) {

    auto viter = values.begin();
    auto vend = values.end();

    for (auto a: argtemplate) {              
        ArgumentString out;
        while (true) {
            auto p = std::min(a.find("{}"), a.size());
            for (std::size_t i = 0; i < p; ++i) out.push_back(static_cast<ArgumentString::value_type>(a[i]));
            if (p >= a.size()) break;
            if (viter != vend) {
                out.append(*viter);
                ++viter;
            }
            a = a.substr(p+2);            
        }
        cont.emplace_back(std::move(out));
    }    
}