export module cairn.utils.simple_json;

import cairn.utils.utf8;

import <variant>;
import <vector>;
import <unordered_map>;
import <string>;
import <format>;
import <optional>;
import <exception>;
import <charconv>;

export class Json ;

using JsonTypes = std::variant<
    std::nullptr_t,
    std::string,
    double,
    bool,
    std::vector<Json>,
    std::unordered_map<std::string, Json> >;


export class Json: public JsonTypes  {
public:
    using JsonTypes::JsonTypes;
    Json() {};
    Json(std::string_view str):JsonTypes(std::string(str)) {}
    Json(std::u8string_view str):JsonTypes(std::string(string_from_u8(str))) {}
    Json(const std::u8string &str):Json(std::u8string_view(str)) {}
    Json(std::wstring_view str) {
        std::u8string s;
        to_utf8(str.begin(), str.end(), std::back_inserter(s));
        *this = Json(s);
    }
    Json(const std::wstring &str):Json(std::wstring_view(str)) {}

    using Object = std::unordered_map<std::string, Json> ;
    using Array = std::vector<Json>;

    bool is_null() const {return std::holds_alternative<std::nullptr_t>(*this);}
    bool is_bool() const {return std::holds_alternative<bool>(*this);}
    bool is_number() const {return std::holds_alternative<double>(*this);}
    bool is_string() const {return std::holds_alternative<std::string>(*this);}    
    bool is_array() const {return std::holds_alternative<Array>(*this);}
    bool is_object() const {return std::holds_alternative<Object>(*this);}

    bool as_bool() const {
        return is_bool()?std::get<bool>(*this):false;
    }
    double as_number() const {
        return is_bool()?std::get<double>(*this):is_string()?std::strtod(std::get<std::string>(*this).c_str(),0):0.0;        
    }
    const std::string &as_string() const {
        if (is_string()) return std::get<std::string>(*this);
        else {
            static std::string empty;
            return empty;
        }
    }

    std::wstring as_wstring() const {
        auto s = as_string();
        std::wstring out;
        from_utf8<wchar_t>(s.begin(), s.end(), std::back_inserter(out));
        return out;
    }

    const Array &as_array() const {
        if (is_array()) return std::get<Array>(*this);
        else {
            static Array empty;
            return empty;
        }
    }
    const Object &as_object() const {
        if (is_object()) return std::get<Object>(*this);
        else {
            static Object empty;
            return empty;
        }
    }

    template<std::invocable<Array &> Fn>
    bool update(Fn &fn) {
        if (!is_array()) *this = Array();
        fn(std::get<Array>(*this));
    }
    template<std::invocable<Object &> Fn>
    bool update(Fn &fn) {
        if (!is_array()) *this = Object();
        fn(std::get<Object>(*this));
    }

    template<std::invocable<char> Fn>
    void serialize(Fn &&fn) const  {
        if (is_null()) write_token(fn,"null");
        else if (is_bool()) write_token(fn, as_bool()?"true":"false");
        else if (is_string()) write_string(fn, as_string());
        else if (is_number()) {
            char buff[50];
            auto iter = std::format_to_n(buff,sizeof(buff), "{:.12g}", as_number());
            write_token(fn, std::string_view{buff, iter.out});
        } else if (is_array()) {
            auto &a = as_array();
            fn('[');
            auto iter = a.begin();
            auto end = a.end();
            if (iter != end) {
                iter->serialize(fn);
                ++iter;
                while (iter != end) {
                    fn(',');
                    iter->serialize(fn);
                    ++iter;
                }
            }
            fn(']');
        } else if (is_object()) {
            auto &o = as_object();
            fn('{');
            auto iter = o.begin();
            auto end = o.end();
            if (iter != end) {
                write_string(fn, iter->first);
                fn(':');
                iter->second.serialize(fn);                
                ++iter;
                while (iter != end) {
                    fn(',');
                    write_string(fn, iter->first);
                    fn(':');
                    iter->second.serialize(fn);                
                    ++iter;
                }
            }
            fn('}');            
        }
    }

    class ParseError: std::exception {
    public:
        virtual const char *what() const noexcept {return "json parse error";}
    };


    template<std::invocable<> Fn>
    requires(std::is_invocable_r_v<std::optional<char>, Fn>)
    static Json parse(Fn &&fn) {     
        char c= read_skip_ws(fn);
        return parse_first_chr(c, fn);
    }

    Json(std::initializer_list<Json> list) {
        bool isobj = std::all_of(list.begin(), list.end(), [](const Json &x){
            if (!x.is_array()) return false;
            auto &arr = x.as_array();            
            return arr.size() == 2 && arr[0].is_string();
        });
        if (isobj) {
            Json::Object obj;
            for (const auto &x: list) {
                auto &arr = x.as_array();
                obj.emplace(arr[0].as_string(), arr[1]);
            }
            *this = std::move(obj);
        } else {
            *this = Json::Array(list.begin(), list.end());
        }
    }


protected:

    using ReadChr = std::optional<char>;

    template<typename Fn>
    static void write_token(Fn &&fn, std::string_view s) {
        for (auto x: s) fn(x);
    }
    template<typename Fn>
    static void write_string(Fn &&fn, std::string_view s) {
        fn('"');
        for (auto x: s) {
            switch (x) {
                case '\n': write_token(fn,"\\n");
                case '\r': write_token(fn,"\\r");
                case '\t': write_token(fn,"\\t");
                case '\f': write_token(fn,"\\f");
                case '\b': write_token(fn,"\\b");
                case '\\': write_token(fn,"\\\\");
                case '\"': write_token(fn,"\\\"");
                default: 
                    if (x >= 0 && x < 32) {
                        char buff[6] = "\\u";
                        auto iter = std::format_to_n(buff+2,4,"{:04x}", x);
                        write_token(fn,{buff,iter.out});
                    } else {
                        fn(x);
                    }
            }
        }
        fn('"');
    }
    template<typename Fn>
    static char read_skip_ws(Fn &&fn) {
        ReadChr c = fn();
        while (c && std::isspace(*c)) {
            c = fn();
        }
        if (!c) throw ParseError();
        return *c;
    }
    template<typename Fn>
    static Json parse_first_chr(char &c, Fn &&fn) {
        switch (c) {
            case 't': check(c, fn, "true"); c = 0; return Json(true);
            case 'f': check(c, fn, "false"); c = 0; return Json(false);
            case 'n': check(c, fn, "null"); c = 0; return Json(nullptr);
            case '"': c =0; return Json(parse_string(fn));
            case '[':  {
                Array arr;
                c = read_skip_ws(fn);
                if (c != ']') {
                    arr.push_back(parse_first_chr(c,fn));
                    if (!c) c = read_skip_ws(fn);
                    while (c != ']') {
                        if (c != ',') throw ParseError();
                        c = read_skip_ws(fn);
                        arr.push_back(parse_first_chr(c, fn));
                        if (!c) c = read_skip_ws(fn);
                    }
                }
                c = 0;
                return Json(std::move(arr));                
            }
            case '{': {
                Object obj;
                c = read_skip_ws(fn);
                if (c != '}') {
                    if (c!='"') throw ParseError();
                    while (true) {
                        std::string k = parse_string(fn);
                        c = read_skip_ws(fn);
                        if (c!=':') throw ParseError();
                        c = read_skip_ws(fn);
                        auto v = parse_first_chr(c, fn);
                        if (!c) c = read_skip_ws(fn);
                        if (!obj.emplace(std::move(k), std::move(v)).second) throw ParseError();
                        if (c == ',') {
                            c = read_skip_ws(fn);
                            continue;
                        } else if (c != '}') {
                            throw ParseError();
                        } else {
                            break;
                        }
                    }
                }
                c = 0;
                return Json(std::move(obj));
            }
            default:
                return Json(parse_number(c, fn));
        }
    }

    template<typename Fn>
    static void check(char c, Fn &&fn, std::string_view token) {
        if (c != token[0]) throw ParseError();
        for (auto &x: token.substr(1)) {
            auto cc = fn();
            if (!cc || *cc != x) throw ParseError();            
        }
    }
    template<typename Fn>
    static double parse_number(char &c, Fn &&fn) {
        std::string buff;
        while (std::isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
            buff.push_back(c);
            auto cc = fn();
            c = !cc?' ': *cc;            
        }
        double v;
        auto st = std::from_chars(buff.data(), buff.data()+buff.size(), v);
        if (st.ec != std::errc{} || st.ptr != buff.data()+buff.size()) throw ParseError();
        if (std::isspace(c)) c = 0;
        return v;
    }
    template<typename Fn>
    static std::string parse_string(Fn &&fn) {
        std::string out;
        char hexbuf[4];
        int m = 0;
        auto cc = fn();
        int surg = 0;
        while (cc && *cc!='"') {
            char c = *cc;
            if (m == 0) {
                if (c == '\\') m = -1;
                else out.push_back(c);
            } else if (m == -1) {
                m = 0;
                switch (c) {
                    case 'n': out.push_back('\n');break;
                    case 'r': out.push_back('\r');break;
                    case 't': out.push_back('\t');break;
                    case 'f': out.push_back('\f');break;
                    case 'a': out.push_back('\a');break;
                    case 'u': m = 1; break;
                    default: out.push_back(c);
                }
            } else if (m > 0) {
                if (!std::isxdigit(c)) throw ParseError();
                hexbuf[m-1] = c;
                if (m == 4) {
                    std::size_t codepoint;
                    std::from_chars(hexbuf,hexbuf+4, codepoint, 16);                    
                    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
                        if (surg) {
                            codepoint = decodeUtf16UnknownOrder(codepoint, surg);
                            surg = 0;
                        } else {
                            surg = codepoint;
                        }
                    }
                    char32_t cp = static_cast<char32_t>(codepoint);
                    to_utf8(&cp, &cp+1, std::back_inserter(out));
                    m = 0;
                } else {
                    ++m;
                }                
            }
             cc = fn();
        }
        return out;
    }
};