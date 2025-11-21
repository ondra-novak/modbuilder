#pragma once

#include <string>
#include <map>
#include <vector>


class SystemEnvironment {
public:

#ifdef _WIN32
    using native_string = std::wstring;
#else  
    using native_string = std::u8string;
#endif

    using CharType = native_string::value_type;
    using native_string_view = std::basic_string_view<CharType>;
    struct Buffer {
        std::vector<CharType> data;
        std::vector<CharType *> pointers;
    };

    SystemEnvironment() = default;
    static SystemEnvironment current();
    static SystemEnvironment parse(std::string_view data);

    CharType **posix_format(Buffer &buff);
    native_string to_windows_format();

    template<typename Me, typename Arch>
    static void serialize(Me &me, Arch &arch) {
        arch(me._env_data);
    }
    
    std::basic_string_view<CharType> operator[](const std::basic_string_view<CharType> &key) const;
    std::basic_string_view<CharType> operator[](const char *key) const;

protected:
    struct Compare {
        bool operator()(native_string_view a, native_string_view b) const;
        using is_transparent = int;
    };
    std::map<native_string, native_string, Compare > _env_data;

};