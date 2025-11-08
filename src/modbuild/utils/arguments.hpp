#pragma once
#include <string>
#include <string_view>
#include <filesystem>


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