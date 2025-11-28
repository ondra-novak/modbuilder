module;
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
extern char **environ;
#endif

export module cairn.utils.env;

import <system_error>;
import <cwctype>;
import <string>;
import <map>;
import <vector>;


export class SystemEnvironment {
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

    CharType **posix_format(Buffer &buff) const;
    native_string to_windows_format() const;

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

SystemEnvironment SystemEnvironment::current() {
    SystemEnvironment env;  
#ifdef _WIN32
    LPWCH envStrings = GetEnvironmentStringsW();
    if (envStrings == nullptr) {
        throw std::system_error(GetLastError(), std::system_category(), "Unable to get environment strings");
    }
    LPWCH current = envStrings;
    while (*current) {
        std::wstring entry = current;
        size_t pos = entry.find(L'=');
        if (pos != std::wstring::npos) {
            std::wstring key = entry.substr(0, pos);
            std::wstring value = entry.substr(pos + 1);
            env._env_data.emplace(std::move(key), std::move(value));
        }
        current += entry.size() + 1;
    }
    FreeEnvironmentStringsW(envStrings);
#else
    for (char **current = environ; *current; ++current) {
        std::string entry = *current;
        size_t pos = entry.find('=');
        if (pos != std::string::npos) {
            std::u8string key = std::u8string(entry.begin(), entry.begin() + pos);
            std::u8string value = std::u8string(entry.begin() + pos + 1, entry.end());
            env._env_data.emplace(std::move(key), std::move(value));
        }
    }
#endif
    return env;
}

SystemEnvironment SystemEnvironment::parse(std::string_view data)
{
    SystemEnvironment env = SystemEnvironment::current();
    #ifdef _WIN32
    // parse SET output format
    // convert to wstring
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, data.data(), (int)data.size(), NULL, 0);
    std::wstring wdata(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, data.data(), (int)data.size(), wdata.data(), size_needed);
    size_t start = 0;
    while (start < wdata.size()) {
        size_t end = wdata.find(L"\r\n", start);
        if (end == std::wstring::npos) {
            end = wdata.size();
        }
        std::wstring line = wdata.substr(start, end - start);
        size_t pos = line.find(L'=');
        if (pos != std::wstring::npos) {
            std::wstring key = line.substr(0, pos);
            std::wstring value = line.substr(pos + 1);
            env._env_data.emplace(std::move(key), std::move(value));
        }
        start = end + 2; // move past \r\n
    }
    #else
    // parse env -0 output format
    size_t start = 0;
    while (start < data.size()) {
        size_t end = data.find('\0', start);
        if (end == std::string_view::npos) {
            end = data.size();
        }
        std::string_view line = data.substr(start, end - start);
        size_t pos = line.find('=');
        if (pos != std::string_view::npos) {
            std::u8string key = std::u8string(line.begin(), line.begin() + pos);
            std::u8string value = std::u8string(line.begin() + pos + 1, line.end());
            env._env_data.emplace(std::move(key), std::move(value));
        }
        start = end + 1; // move past \n
    }
    #endif

    return env;
}

SystemEnvironment::CharType **SystemEnvironment::posix_format(Buffer &buff) const
{   
    //creates array of pointers to "key=value" strings in buff
    buff.pointers.clear ();
    buff.data.clear();
    std::vector<std::size_t> offsets;
    for (const auto & [key, value] : _env_data) {
        size_t start = buff.data.size();
        buff.data.insert(buff.data.end(), key.begin(), key.end());
        buff.data.push_back(static_cast<CharType>('='));
        buff.data.insert(buff.data.end(), value.begin(), value.end());
        buff.data.push_back(static_cast<CharType>(0)); //null terminator
        offsets.push_back(start);
    }
    //null terminator for array
    buff.pointers.reserve(offsets.size() + 1);  
    for (const auto & off : offsets) {
        buff.pointers.push_back(&buff.data[off]);
    }
    buff.pointers.push_back(nullptr); // null terminator for the array
    return buff.pointers.data();
}

SystemEnvironment::native_string SystemEnvironment::to_windows_format() const
{
    //creates double-null-terminated block of "key=value" strings
    native_string result;
    for (const auto & [key, value] : _env_data) {
        result.append(key);
        result.push_back(static_cast<CharType>('='));
        result.append(value);
        result.push_back(static_cast<CharType>(0)); //null terminator
    }
    result.push_back(static_cast<CharType>(0)); // double null terminator
    return result;
}

std::basic_string_view<SystemEnvironment::CharType> SystemEnvironment::operator[](const std::basic_string_view<CharType> &key) const
{
    auto iter = _env_data.find(key);
    if (iter == _env_data.end()) return {};
    return iter->second;
}

std::basic_string_view<SystemEnvironment::CharType> SystemEnvironment::operator[](const char *key) const
{
    std::string_view skey(key);
    native_string s(skey.begin(),skey.end());
    return this->operator[](s);
}

bool SystemEnvironment::Compare::operator()(native_string_view a, native_string_view b) const
{
    std::size_t sz = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < sz; ++i) {
        auto ac = a[i];
        auto bc = b[i];
        #ifdef _WIN32 
        ac = static_cast<decltype(ac)>(std::towlower(ac));
        bc = static_cast<decltype(bc)>(std::towlower(bc));
        #endif
        if (ac < bc) return true;
        if (ac > bc) return false;
    }
    return  (a.size() < b.size());
}
