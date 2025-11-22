#include "compile_commands_supp.hpp"
#include "utils/arguments.hpp"
#include "utils/nlohmann_json.hpp"
#include "utils/utf_8.hpp"
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>


void CompileCommandsTable::load(std::filesystem::path p)
{
    std::ifstream f(p);
    if (f.is_open()) {
        nlohmann::json jdata;
        f >> jdata;
        for (auto v: jdata) {            
            CCRecord rc;
            rc.original_json = v;

            auto jf = v.find("file");
            auto joutput = v.find("output");
            rc.file = jf != v.end() ? std::filesystem::path(u8_from_string(jf->get<std::string_view>())) : std::filesystem::path();
            rc.output = joutput != v.end() ? std::filesystem::path(u8_from_string(joutput->get<std::string_view>())) : std::filesystem::path();

            if (std::filesystem::exists(rc.file)) {
                update(std::move(rc));
            }
        }
    }
}

auto compatible_string(ArgumentStringView other) {
    if constexpr(std::is_same_v<ArgumentStringView::value_type, char8_t>) {
        return string_from_u8(other);
    } else {
        return other;
    }
}

nlohmann::json CompileCommandsTable::export_db() {
    nlohmann::json out = nlohmann::json::array();
    for (const auto &[_,rc]: _table) {
        if (!rc.original_json.is_null()) {
            out.push_back(rc.original_json);
        } else {
            nlohmann::json args = nlohmann::json::array();
            for (const auto &a: rc.arguments) {
                args.push_back(compatible_string(a));
            }
            out.push_back({
                {"command", compatible_string(rc.command)},
                {"file",string_from_u8(rc.file.u8string())},
                {"directory", string_from_u8(rc.directory.u8string())},
                {"arguments",std::move(args)},
                {"output",string_from_u8(rc.output.u8string())}
            });
        }
    }
    return out;

}
void CompileCommandsTable::save(std::filesystem::path p) {
    auto db = export_db();
    std::ofstream out(p, std::ios::out| std::ios::trunc);
    out << db;
}

void CompileCommandsTable::update(CCRecord rec) {
    _table[Key(rec.file, rec.output)] = rec;
}

template <typename CharT, typename Traits = std::char_traits<CharT>>
struct escape {
     using string_view_type = std::basic_string_view<CharT, Traits>;
     
     static constexpr auto qt =static_cast<CharT>('"');
     static constexpr auto space =static_cast<CharT>(' ');
     static constexpr auto sl =static_cast<CharT>('\\');

     escape(string_view_type w):_w(w) {}

     template<typename IO>
     friend IO &operator<<(IO &out, const escape &me) {
            if (me._w.find(space) == me._w.npos) {
                out << me._w;
            } else {
                out.put(qt);
                for (auto c: me._w) {
                    if (c == qt || c == sl) out.put(sl);
                    out.put(static_cast<CharT>(c));
                }
                out.put(qt);
            }
            return out;
     }
     string_view_type _w;
};

template<typename C, typename Traits>
escape(std::basic_string<C, Traits>) -> escape<C, Traits>;

CompileCommandsTable::CCRecord CompileCommandsTable::record(std::filesystem::path directory, 
    std::filesystem::path file, 
    std::vector<ArgumentString> arguments, 
    std::filesystem::path output) {

    auto space = static_cast<ArgumentString::value_type>(' ');

    std::basic_ostringstream<ArgumentString::value_type> command;
    if (!arguments.empty()) {
        command << escape(arguments[0]) ;
        for (std::size_t i = 1; i < arguments.size(); ++i) {
            command << space << escape(arguments[i]) ;
        }
    }

    return CCRecord{std::move(directory),
                    std::move(file),
                    std::move(arguments), 
                    std::move(command).str(), 
                    std::move(output)};
}

CompileCommandsTable::CCRecord CompileCommandsTable::record(std::filesystem::path directory, 
            std::filesystem::path file, 
            std::filesystem::path compiler, 
            std::vector<ArgumentString> arguments, 
            std::filesystem::path output)
{
    arguments.insert(arguments.begin(), path_arg(compiler));

    return record(std::move(directory), std::move(file),
                 std::move(arguments), std::move(output));
}


