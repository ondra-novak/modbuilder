#include "scanner.hpp"
#include <algorithm>


enum class TokenType {
    keyword,
    string,
    symbol,
    angled_include,
    end
};

struct Token {
    TokenType type;
    std::string_view text;
};

bool starts_with(auto a_pos, auto a_end, auto b_pos, auto b_end)
{
    for (; a_pos != a_end; ++a_pos, ++b_pos)
    {
        if (b_pos == b_end || *a_pos != *b_pos)
            return false;
    }
    return true;
}

auto skip_multiline_string(auto pos, auto end) {
    std::string term = ")";
    while (pos != end && *pos != '"' && *pos != '(') {
        term.push_back(*pos);
        ++pos;
    }
    if (pos == end) return pos;
    if (*pos == '"') return pos+1;
    term.push_back('"');    
    while (pos != end) {
        auto x = std::find(pos, end, term.front());
        if (x != end) {
            if (starts_with(term.begin(), term.end(), x, end)) {
                x+=term.size();
                return x;
            }
            ++x;
        }
        pos = x;        
    }
    return pos;
}

auto skip_string(auto pos, auto end) {
    while (pos != end) {
        char c = *pos;
        ++pos;
        if (c == '\\') {
            ++pos;
            if (pos == end) break;
        } else if (c == '"') break;
    }
    return pos;
}

auto parse_string(std::vector<char> &buff, auto pos, auto end) {
    while (pos != end) {
        char c = *pos;
        ++pos;
        if (c == '\\') {
            buff.push_back(c);
            if (pos == end) break;
            buff.push_back(*pos);
            ++pos;
        } else if (c == '"') break;
        buff.push_back(c);
    }
    return pos;
}

auto parse_header_angled(std::vector<char> &buff, auto pos, auto end) {
    while (pos != end) {
        char c = *pos;
        ++pos;
        if (c == '>') break;
        buff.push_back(c);
    }
    return pos;
}

auto skip_braces(char t, auto pos, auto end) -> decltype(pos) {
    while (pos != end) {
        char c = *pos;
        ++pos;
        if (c == t) {
            break;
        }
        switch (c) {
            case '(':  pos = skip_braces(')', pos, end); break;
            case '{':  pos = skip_braces('}', pos, end); break;
            case '[':  pos = skip_braces(']', pos, end); break;
            case '"':  pos = skip_string( pos, end); break;
            default: break;
        }
    }
    return pos;
}

auto simple_tokenizer(std::string_view text) {
    return [
        pos = text.begin(),
        end = text.end(),
        buff = std::vector<char>()
    ](bool header_angled = false) mutable -> Token {

        buff.clear();

        while (pos != end) {
            char c = *pos;
            if (std::isalnum(c) || c == '_' || c == ':' || c == '.') {
                buff.push_back(c);
                ++pos;
            } else {
                if (c == '"' && !buff.empty() && buff.back() == 'R')  {
                    buff.clear();
                    pos = skip_multiline_string(pos+1, end);
                } else if (!buff.empty()) {
                    return {TokenType::keyword, std::string_view(buff.begin(), buff.end())};
                } else {
                    ++pos;
                    switch (c){
                        case '(': pos = skip_braces(')', pos, end);break;
                        case '{': pos = skip_braces('}', pos, end);break;
                        case '[': pos = skip_braces(']', pos, end);break;
                        case '"': pos = parse_string(buff, pos, end);
                            return {TokenType::string, std::string_view(buff.begin(), buff.end())};
                        case '<': if (header_angled) {
                            pos = parse_header_angled(buff, pos, end);
                            return {TokenType::angled_include, std::string_view(buff.begin(), buff.end())};
                        } break;                        
                        default:if (!std::isspace(c)) {
                                        buff.push_back(c);
                                        return {TokenType::symbol, std::string_view(buff.begin(), buff.end())};
                                }                    
                                break;
                    }
                }
            }
        }
        if (!buff.empty()) {
            return {TokenType::keyword, std::string_view(buff.begin(), buff.end())};
        }
        return {TokenType::end,{}};
    };

}


void uniq(auto &cont) {
    if (cont.empty()) return;
    std::sort(cont.begin(), cont.end());
    auto b = cont.begin();    
    auto bb = b;
    ++bb;
    auto e =std::remove_if(bb, cont.end(), [&](const auto &v){
        if (*b == v) return true;
        ++b;
        return false;
    });
    cont.erase(e, cont.end());
}

SourceScanner::Info SourceScanner::scan_string(const std::string_view text) {

    auto r = scan_string_2(text);
    uniq(r.exported);
    uniq(r.required);
    uniq(r.system_headers);
    uniq(r.user_headers);
    return r;

}

std::string handle_partition(const std::string &name, std::string_view part) {
    //assume part not empty;
    std::string ret;
    if (part.front() != ':') {
        ret.append(part);
    } else {
        ret.append(name);
        auto sep = ret.rfind(':');
        if (sep != ret.npos) {
            ret.resize(sep);            
        }
        ret.append(part);
    }
    return ret;
}

static inline bool is_paritition(std::string_view name) {
    return name.find(':') != name.npos;
}

SourceScanner::Info SourceScanner::scan_string_2(const std::string_view text) {

    Info nfo;

    auto tkn = simple_tokenizer(text);
    bool cont;

    bool has_export = false;

    do {
        cont = true;      
        auto s = tkn();

        if (s.type == TokenType::keyword) {
            if (s.text == "module") {
                s = tkn();
                if (s.type == TokenType::keyword) { 
                    nfo.name = s.text;
                    nfo.type = is_paritition(s.text)?ModuleType::partition
                              :has_export?ModuleType::interface:ModuleType::implementation;
                    cont = false;
                    has_export = false;
                }               
            } else if (s.text == "export") {
                has_export = true;
                continue;
            } else {
                if (s.text == "import") {
                    s = tkn(true);
                    if (s.type == TokenType::keyword)  { 
                        nfo.required.push_back(std::string(s.text));
                        cont = false;
                    } else if (s.type == TokenType::string) {
                        nfo.user_headers.push_back(std::string(s.text));
                    } else  if (s.type == TokenType::angled_include) {
                        nfo.system_headers.push_back(std::string(s.text));
                    }
                }
            }
        }
        has_export = false;
        if (s.type == TokenType::end) return nfo;

    } while (cont);
    //now parse imports

    do {
        auto s = tkn();

        if (s.type == TokenType::keyword) {

            if (s.text == "export") {
                has_export = true;
                continue;
            }
            if (s.text == "import") {
                s = tkn(true);
                if (s.type == TokenType::keyword) {
                    auto n = handle_partition(nfo.name, s.text);
                    nfo.required.push_back(std::move(n));
                    if (has_export) {
                        nfo.exported.push_back(nfo.required.back());
                        has_export = false;  
                    }                 
                } else if (s.type == TokenType::string) {
                    nfo.user_headers.push_back(std::string(s.text));
                } else  if (s.type == TokenType::angled_include) {
                    nfo.system_headers.push_back(std::string(s.text));
                }                
            }
        }
        has_export = false;
        if (s.type == TokenType::end) return nfo;
    } while (true);

}
