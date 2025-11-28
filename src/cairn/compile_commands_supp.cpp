module cairn.compile_commands;

import cairn.utils.arguments;
import cairn.utils.hash;
import cairn.utils.utf8;
import cairn.utils.simple_json;

import <filesystem>;
import <fstream>;
import <ostream>;
import <sstream>;
import <string>;
import <string_view>;
import <filesystem>;
import <unordered_map>;
import <vector>;
import <variant>;
import <optional>;
import <exception>;


void CompileCommandsTable::load(std::filesystem::path p)
{
    std::ifstream f(p);
    if (f.is_open()) {
        Json jdata = Json::parse([&]()->std::optional<char> {
            int c = f.get();
            if (c == -1) return std::nullopt;
            else return static_cast<char>(c);
        });
        for (auto &v: jdata.as_array()) {            
            CCRecord rc;
            rc.original_json = v;

            auto obj = v.as_object();            

            auto jf = obj.find("file");
            auto joutput = obj.find("output");
            rc.file = jf != obj.end() ? std::filesystem::path(u8_from_string(jf->second.as_string())) : std::filesystem::path();
            rc.output = joutput != obj.end() ? std::filesystem::path(u8_from_string(joutput->second.as_string())) : std::filesystem::path();

            if (std::filesystem::exists(rc.file)) {
                update(std::move(rc));
            }
        }
    }
}

Json CompileCommandsTable::export_db() {
    Json::Array out;
    for (const auto &[_,rc]: _table) {
        if (!rc.original_json.is_null()) {
            out.push_back(rc.original_json);
        } else {
            Json::Array args;
            for (const auto &a: rc.arguments) {
                args.emplace_back(a);
            }
            out.push_back({
                {"command", rc.command},
                {"file",rc.file.u8string()},
                {"directory", rc.directory.u8string()},
                {"arguments",std::move(args)},
                {"output",rc.output.u8string()}
            });
        }
    }
    return Json(std::move(out));

}
void CompileCommandsTable::save(std::filesystem::path p) {
    auto db = export_db();
    std::ofstream out(p, std::ios::out| std::ios::trunc);
    db.serialize([&](char c){
        out.put(c);
    });
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


