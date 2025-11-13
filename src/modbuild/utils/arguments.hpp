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

template<typename T>
struct ArgumentDef {
    char short_switch;
    std::string_view long_switch;
    std::variant<
        bool (T::*)(),    //switch activate,
        bool (T::*)(int),    //switch integer
        bool (T::*)(std::string),    //switch string
        bool (T::*)(ArgumentStringView),    //switch string
        bool (T::*)(std::filesystem::path path) //path string
        > action;
    std::string_view name;
    std::string_view help;
};

template<typename T>
bool process_arguments(std::span<const ArgumentDef<T> > defs, T &target, CliReader<ArgumentStringView::value_type> &rd) {

    decltype(defs.begin()) itr, prev_iter = defs.begin();
    std::string sw;
    auto cwd = std::filesystem::current_path();

    do {
        auto item = rd.next();
        if (item.is_end) return true;        
        else if (item.is_short_sw) {
            itr =std::find_if(defs.begin(), defs.end(), [&](const auto &d){return d.short_switch == item.short_sw;});
            if (itr == defs.end()) {
                std::cerr << "Unexpected short switch :" << static_cast<char>(item.short_sw) << std::endl;
                return false;
            } 
        } else if (item.is_long_sw) {
            sw.clear();
            std::basic_string_view org_sw(item.long_sw);
            std::copy(org_sw.begin(), org_sw.end(), std::back_inserter(sw));
            itr =std::find_if(defs.begin(), defs.end(), [&](const auto &d){return d.long_switch == sw;});
            if (itr == defs.end()) {
                std::cerr << "Unexpected long switch :" << sw << std::endl;
                return false;
            } 
        } else {
            itr =std::find_if(prev_iter, defs.end(), [&](const auto &d){
                return d.long_switch.empty() && d.short_switch == 0
                    && ( std::holds_alternative<bool (T::*)(int)>(d.action)
                        ||  std::holds_alternative<bool (T::*)(std::string)>(d.action)
                        ||  std::holds_alternative<bool (T::*)(ArgumentStringView)>(d.action)
                        ||  std::holds_alternative<bool (T::*)(std::filesystem::path)>(d.action));
            });
            if (itr == defs.end()) {
                sw.clear();
                std::basic_string_view org_sw(item.long_sw);
                std::copy(org_sw.begin(), org_sw.end(), std::back_inserter(sw));
                std::cerr << "Unexpected command line argument:" << sw << std::endl;
                return false;
            } 
            prev_iter = itr+1;
            rd.put_back();
        }
        if (rd) {
            if (std::holds_alternative<bool (T::*)(int)>(itr->action)) {
                auto a =std::get<bool (T::*)(int)>(itr->action);
                if (!(target.*a)(rd.number())) return true;
            }else if (std::holds_alternative<bool (T::*)(std::filesystem::path)>(itr->action)) {            
                auto a=std::get<bool (T::*)(std::filesystem::path)>(itr->action);
                if (!(target.*a)((cwd/rd.text()).lexically_normal())) return true;
            }else if (std::holds_alternative<bool (T::*)(std::string)>(itr->action)) {            
                auto a = std::get<bool (T::*)(std::string)>(itr->action);
                sw.clear();
                std::basic_string_view org_sw(rd.text());
                std::copy(org_sw.begin(), org_sw.end(), std::back_inserter(sw));
                if (!(target.*a)(sw)) return true;
            }else if (std::holds_alternative<bool (T::*)(ArgumentStringView)>(itr->action)) {            
                auto a=std::get<bool (T::*)(ArgumentStringView)>(itr->action);
                if (!(target.*a)(rd.text())) return true;
            }
        } else {
            if (std::holds_alternative<bool (T::*)()>(itr->action)) {
                auto a = std::get<bool (T::*)()>(itr->action);
                if (!(target.*a)()) return true;
            } else {
                std::cerr << "Expected extra command line argument." << std::endl;
                return false;
            }
        }    

    } while (true);

}



