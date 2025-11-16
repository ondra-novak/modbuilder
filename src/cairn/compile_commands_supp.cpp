#include "compile_commands_supp.hpp"
#include "utils/arguments.hpp"
#include "json/value.h"
#include "json/parser.h"
#include "json/serializer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

void CompileCommandsTable::load(std::filesystem::path p) {
    std::ifstream f(p);
    if (f.is_open()) {
        auto data = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        auto jdata = json::value::from_json(data);
        for (auto v: jdata) {            
            CCRecord rc;
            rc.original_json = v;
            rc.file = v["file"].as<std::u8string>();
            if (std::filesystem::exists(rc.file)) {
                update(std::move(rc));
            }
        }
    }
}


json::value CompileCommandsTable::export_db() {
    return json::value(_table.begin(), _table.end(),  
        [&](const auto &rc) {
            if (rc.second.original_json.defined()) {
                return rc.second.original_json;
            }
            return json::value{
                {"command", rc.second.command},
                {"file", rc.second.file.u8string()},
                {"directory", rc.second.directory.u8string()},
                {"arguments",json::value(
                    rc.second.arguments.begin(), rc.second.arguments.end(), [](const ArgumentString &s){
                        return json::value(s);
                    }
                )}
            };
        });

}
void CompileCommandsTable::save(std::filesystem::path p) {
    auto str = export_db().to_json();
    std::ofstream out(p, std::ios::out| std::ios::trunc);
    out << str;    
}

void CompileCommandsTable::update(CCRecord rec) {
    _table[rec.file] = rec;
}

template <typename CharT, typename Traits = std::char_traits<CharT>>
struct escape {
     using string_view_type = std::basic_string_view<CharT, Traits>;
     
     static constexpr auto qt =static_cast<CharT>('"');
     static constexpr auto sl =static_cast<CharT>('\\');

     escape(string_view_type w):_w(w) {}

     template<typename IO>
     friend IO &operator<<(IO &out, const escape &me) {
            for (auto c: me._w) {
                if (c == qt || c == sl) out.put(sl);
                out.put(static_cast<CharT>(c));
            }
            return out;
     }
     string_view_type _w;
};

template<typename C, typename Traits>
escape(std::basic_string<C, Traits>) -> escape<C, Traits>;

CompileCommandsTable::CCRecord CompileCommandsTable::record(std::filesystem::path directory, 
    std::filesystem::path file, 
    std::vector<ArgumentString> arguments) {

    std::basic_ostringstream<ArgumentString::value_type> command;
    if (!arguments.empty()) {
        command << static_cast<ArgumentString::value_type>('"') << escape(arguments[0]) << static_cast<ArgumentString::value_type>('"');
        for (std::size_t i = 1; i < arguments.size(); ++i) {
            command << static_cast<ArgumentString::value_type>(' ') 
                    << static_cast<ArgumentString::value_type>('"') 
                    << escape(arguments[i]) << static_cast<ArgumentString::value_type>('"');
        }
    }

    return CCRecord{std::move(directory), std::move(file), std::move(arguments), std::move(command).str()};
}


