#include "preprocess.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>



template<typename Fn>
std::string read_string(Fn &&fn, std::string beg) {
    bool slash = false;
    int c = fn();
    while (c != -1 && (slash || c != '"')) {
        beg.push_back(static_cast<char>(c));
        slash = c == '\\';
        c = fn();
    }
    beg.push_back('\"');
    return beg;
}

template<typename Fn>
auto StupidPreprocessor::tokenizer(Fn &&fn) {
    return [fn = std::forward<Fn>(fn), cur = Token()]() mutable {        
        if (cur.type == TokenType::eof) return cur;
        while (true) {
            int c = fn();
            if (c == -1) {
                return std::exchange(cur, Token {TokenType::eof});
            }
            char cc = static_cast<char>(c);
            if (std::isspace(c)) {
                if (cur.type == TokenType::white) {
                    cur.content.push_back(cc);
                } else {
                    return std::exchange(cur,Token{TokenType::white, {&cc,1}});
                }
            } else if (c == '_' || std::isalnum(c)) {
                if ((std::isdigit(c) && cur.type == TokenType::number) 
                    || (std::isxdigit(c) && cur.type == TokenType::xnumber)
                    || cur.type == TokenType::identifier) {
                        cur.content.push_back(cc);                                        
                } else if (cur.type == TokenType::number && cur.content.front()=='0' && c == 'x') {
                    cur.type = TokenType::xnumber;
                    cur.content.push_back(cc);
                } else if (std::isdigit(c)) {
                    return std::exchange(cur, Token{TokenType::number, {&cc,1}});
                } else {
                    return std::exchange(cur, Token{TokenType::identifier, {&cc,1}});
                }
            } else if (c == '"') {
                return std::exchange(cur, Token{TokenType::string, read_string(fn,{&cc,1})});
            
            } else {
                if (c == '+' || c == '-' || c=='!' || cur.type != TokenType::symbol) {                    
                    return std::exchange(cur, Token{TokenType::symbol, {&cc,1}});
                } else {
                    cur.content.push_back(c);
                }
            }
        }
    };
}

auto StupidPreprocessor::tokenizer_from_stream(std::span<const Token> stream) {
    return [stream]() mutable {
        if (stream.empty()) return Token{TokenType::eof, {}};
        else {
            auto r = stream.front();
            stream = stream.subspan(1);
            return r;
        }
    };
}

auto StupidPreprocessor::tokenizer_from_string(std::string_view line) {
    return tokenizer([=]()mutable {
        if (line.empty()) return -1;
        int r = static_cast<unsigned char>(line.front());
        line = line.substr(1);
        return r;
    });
}

auto stream_reader(std::istream &in) {
    return [&]() mutable {
        return in.get();
    };
}


template<typename Source>
auto unwrap_lines(Source &&src) {

    return [src = std::forward<Source>(src), extra = std::queue<int>() ]() mutable {
        while (true) {
            if (!extra.empty()) {
                int r = extra.front();
                extra.pop();
                return r;
            }
            int c = src();
            if (c != '\\') return c;
            extra.push(c);
            do {
                c = src();            
                extra.push(c);
            } while (c != -1 && c != '\n' && std::isspace(c));
            if (c == -1) {
                extra = {};
                return c;
            }
            if (c == '\n') {
                extra = {};
            }
        }
    };
}

template<typename Source>
int skip_block_comment(Source &&src){
    bool st = false;
    while (true) {
        int c = src();
        if (c == -1) return -1;
        if (c == '*') st = true;
        else if (c == '/' && st) {
            return src();
        } else {
            st = false;
        }
    }
}

template<typename Source>
int skip_line_comment(Source &&src){
    while (true) {
        int c = src();
        if (c == -1 || c == '\n') return c;
    }
}

template<typename Source>
auto remove_comments(Source &&src) {

    return [src = std::forward<Source>(src), tmp = -1]() mutable  {
        if (tmp != -1) {
            int r = tmp;
            tmp = -1;
            return r;
        }
        int c = src();
        if (c == '/') {
            int d = src();
            if (d == '*') return skip_block_comment(src);
            if (d == '/') return skip_line_comment(src);            
            tmp = d;            
        }
        return c;        
    };
}

template<typename Source>
bool get_line(Source &&src, std::string &ln) {
    ln.clear();
    int c = src();
    while (c != -1 && c != '\n') {
        if (c != '\r') ln.push_back(static_cast<char>(c));
        c = src();
    }
    return c != -1;
}

static std::string_view trim(std::string_view x) {
    while (!x.empty() && std::isspace(x.front())) x = x.substr(1);
    while (!x.empty() && std::isspace(x.back())) x = x.substr(0, x.length()-1);
    return x;
}

StupidPreprocessor::NextCommand StupidPreprocessor::run(const std::filesystem::path &cur_dir, std::istream &in, ScanMode mode, std::ostream &out) {
    auto src = remove_comments(unwrap_lines(stream_reader(in)));
    std::string ln;
    bool st;
    do {
        st = get_line(src, ln);
        auto lnv = trim(ln);
        auto sep = std::min(lnv.find(' '), lnv.size());
        auto cmdstr = lnv.substr(0,sep);
        auto args = trim(lnv.substr(sep));
        auto cmd = find(cmdstr) ;        
        if (cmd == Command::eof) {
            if (mode == ScanMode::copy) {
                out << ln << std::endl;     //we don't expand macros in code
            }
        } else {
            std::optional<bool> cond = {};
            switch (cmd) {
                case Command::_endif:
                case Command::_else:
                case Command::_elif:
                case Command::_elifdef: return {cmd, std::string(args)};
                case Command::_define: if (mode != ScanMode::skip) parse_define(args);break;
                case Command::_undef: if (mode != ScanMode::skip) parse_undef(args);break;
                case Command::_include:if (mode != ScanMode::skip) parse_include(cur_dir, args);break;
                case Command::_if: cond = parse_if(args);break;
                case Command::_ifdef: cond = parse_ifdef(args);break;
                case Command::_ifndef: cond = !parse_ifdef(args);break;
                default: break;
            }
            if (cond.has_value()) {
                bool res = *cond;
                auto r  = run(cur_dir, in, res?mode:ScanMode::skip, out);
                while (r.cmd != Command::eof && r.cmd != Command::_endif) {
                    switch (r.cmd) {
                        case Command::_else:  res = !res;break;
                        case Command::_elif: cond = parse_if(r.args);break;
                        case Command::_elifdef: cond = parse_ifdef(r.args);break;
                        case Command::_elifndef: cond = !parse_ifdef(r.args);break;
                        default: cond = false; break;
                    }
                    r  = run(cur_dir, in, res?mode:ScanMode::skip, out);
                }
            }
        }
    } while (st);
    return {Command::eof, {}};
}

void StupidPreprocessor::parse_define(std::string_view args){
    auto next_token = tokenizer_from_string(args);
    next_token();   //drop begin;
    auto name = next_token();
    if (name.type == TokenType::eof) return;
    MacroDef def;
    Token f = next_token();
    if (f.content =="(") {
        Token c = next_token();
        def.variables.emplace();
        while (c.type != TokenType::eof && c.content != ")") {
            if (c.type == TokenType::identifier) def.variables->push_back(c.content);
            c = next_token();
        }    
    } else if (f.type != TokenType::white) {
        return;
    }
    f = next_token();
    while (f.type != TokenType::eof) {
        def.content.push_back(f);
        f = next_token();
    }
    _context[name.content] = def;

}
void StupidPreprocessor::parse_undef(std::string_view args){
    auto next_token = tokenizer_from_string(args);
    next_token();   //drop begin;
    auto name = next_token();
    if (name.type == TokenType::eof) return;
    _context.erase(name.content);
    
}
void StupidPreprocessor::parse_include(const std::filesystem::path &cur_dir, std::string_view args){
    auto next_token = tokenizer_from_string(args);
    std::string path;
    next_token();   //drop begin;
    Token t = next_token();
    std::string end;
    if (t.content == "<") end = ">";
    else if (t.content == "\"") end = "\"";
    else return;

    t = next_token();
    while (t.type != TokenType::eof && t.content != end) {
        path.append(t.content);
        t = next_token();
    }

    std::filesystem::path final_path;
    final_path = (cur_dir/path).lexically_normal();
    if (end.front() != '"') {
        for (auto &i: _includes) {
            std::error_code ec;
            auto cand = std::filesystem::canonical(i/path, ec);
            if (ec == std::errc{}) {
                final_path = std::move(cand);
                break;
            }            
        }
    }

    std::ifstream file(final_path);
    if (file.is_open()) {
        std::ostringstream dummy;
        run(final_path.parent_path(), file, ScanMode::collect, dummy);
    }    
}

std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_oror(auto &&next_token) {
    auto a = evaluate_andand(next_token);
    if (a.second.content == "||") {
        auto b = evaluate_oror(next_token);
        return {(a.first || b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_andand(auto &&next_token) {
    auto a = evaluate_or(next_token);
    if (a.second.content == "&&") {
        auto b = evaluate_andand(next_token);
        return {(a.first && b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_or(auto &&next_token) {
    auto a = evaluate_xor(next_token);
    if (a.second.content == "|") {
        auto b = evaluate_or(next_token);
        return {(a.first | b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_xor(auto &&next_token) {
    auto a = evaluate_and(next_token);
    if (a.second.content == "^") {
        auto b = evaluate_xor(next_token);
        return {(a.first ^ b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_and(auto &&next_token) {
    auto a = evaluate_equal(next_token);
    if (a.second.content == "&") {
        auto b = evaluate_and(next_token);
        return {(a.first & b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_equal(auto &&next_token) {
    auto a = evaluate_relation(next_token);
    if (a.second.content == "==") {
        auto b = evaluate_equal(next_token);
        return {(a.first == b.first), b.second};
    }
    if (a.second.content == "!=") {
        auto b = evaluate_equal(next_token);
        return {(a.first != b.first), b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_relation(auto &&next_token) {
    auto a = evaluate_shift(next_token);
    if (a.second.content == "<") {
        auto b = evaluate_relation(next_token);
        return {(a.first < b.first), b.second};
    }
    if (a.second.content == ">") {
        auto b = evaluate_relation(next_token);
        return {(a.first > b.first), b.second};
    }
    if (a.second.content == "<=") {
        auto b = evaluate_relation(next_token);
        return {(a.first <= b.first), b.second};
    }
    if (a.second.content == ">=") {
        auto b = evaluate_relation(next_token);
        return {(a.first >= b.first), b.second};
    }
    return a;
}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_shift(auto &&next_token) {
    auto a = evaluate_sum(next_token);
    if (a.second.content == "<<") {
        auto b = evaluate_shift(next_token);
        return {(a.first << b.first), b.second};
    }
    if (a.second.content == ">>") {
        auto b = evaluate_shift(next_token);
        return {(a.first >> b.first), b.second};
    }
    return a;
}

std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_sum(auto &&next_token) {
    auto a = evaluate_multiply(next_token);
    if (a.second.content == "+") {
        auto b = evaluate_sum(next_token);
        return {a.first + b.first, b.second};
    }
    if (a.second.content == "-") {
        auto b = evaluate_sum(next_token);
        return {a.first - b.first, b.second};
    }
    return a;
}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_multiply(auto &&next_token) {
    auto a = evaluate_unar(next_token);
    if (a.second.content == "*") {
        auto b = evaluate_multiply(next_token);
        return {a.first * b.first, b.second};
    }
    if (a.second.content == "/") {
        auto b = evaluate_sum(next_token);
        if (b.first == 0) return {0, b.second};
        return {a.first / b.first, b.second};
    }
    return a;

}
std::pair<long, StupidPreprocessor::Token> StupidPreprocessor::evaluate_unar(auto &&next_token) {

    auto fetch_next = [&]{
        Token k = next_token();
        while (k.type == TokenType::white) k = next_token();
        return k;
    };

    Token t = fetch_next();
    if (t.content == "-") {
        auto b = evaluate_unar(next_token);
        return {-b.first, b.second};
    } 
    if (t.content == "!") {
        auto b = evaluate_unar(next_token);
        return {b.first?0:1, b.second};
    } 
    if (t.content == "(") {
        auto b = evaluate_oror(next_token);
        auto t = b.second;
        if (t.content == ")") b.second = fetch_next();
        return b;
    }
    if (t.type == TokenType::number) {
        auto l = std::strtol(t.content.c_str(), NULL,t.content.front() == '0'?8:10);
        return {l, fetch_next()};        
    }
    if (t.type == TokenType::xnumber) {
        auto l = std::strtol(t.content.c_str()+2, NULL,16);
        return {l, fetch_next()};        
    }
    return {0,t};
}


long StupidPreprocessor::evaluate(auto &&next_token) {
    long a = evaluate_oror(next_token).first;
    return a;
}

bool StupidPreprocessor::parse_if(std::string_view args){
    std::vector<Token> stream;
    std::unordered_set<std::string> dummy;
    preproc_line(tokenizer_from_string(args), stream, {}, dummy, true);
    long result = evaluate(tokenizer_from_stream(stream));
    return result != 0;
}

bool StupidPreprocessor::parse_ifdef(std::string_view args){
    auto next_token = tokenizer_from_string(args);
    next_token();   //drop begin;
    auto name = next_token();
    if (name.type == TokenType::eof) return false;
    return _context.contains(name.content);

}



template<typename Fn>
void StupidPreprocessor::preproc_line(Fn &&next_token, std::vector<Token> &token_stream, const MacroMap &arguments, std::unordered_set<std::string> &deactivated, bool cond) const {
    Token t = next_token();
    while (t.type != TokenType::eof) {
        if (t.type != TokenType::begin && t.type != TokenType::white) {
            MacroMap::const_iterator mvaritr;
            if (t.type == TokenType::identifier) {
                if (cond && t.content == "defined") {

                    Token u = next_token();                
                    while (u.type == TokenType::white) u = next_token();
                    Token v;                    
                    if (u.content == "(") {
                        Token v = next_token();
                        while (v.type == TokenType::white) v = next_token();
                    } else {
                        std::swap(u,v);
                    }
                    if (_context.contains(v.content)) {
                        token_stream.push_back({TokenType::number,"1"});
                    } else {
                        token_stream.push_back({TokenType::number,"0"});
                    }
                    if (u.type != TokenType::begin) {
                        u = next_token();
                        while (u.type != TokenType::eof && u.content != ")") u = next_token();
                    }
                } else if ((mvaritr = arguments.find(t.content)) != arguments.end()) {
                    //not supporting expansion of function inside expansion - too much clever
                    preproc_line(tokenizer_from_stream(mvaritr->second.content), token_stream, {}, deactivated, cond);
                } else if (deactivated.find(t.content) == deactivated.end()) {
                    auto iter = _context.find(t.content);
                    if (iter != _context.end()) {

                        if (iter->second.variables.has_value()) {
                            Token c = next_token();
                            while (c.type == TokenType::white) c = next_token();
                            if (c.content == "(") {
                                MacroMap new_args;
                                auto atr = iter->second.variables->begin();
                                c = next_token();
                                int level = 1;
                                auto strm = &new_args[*atr].content;
                                while (c.type != TokenType::eof && level) {
                                    c = next_token();
                                    if (level ==0 && c.content ==",") {
                                        auto tmp = atr;
                                        ++tmp;
                                        if (tmp != iter->second.variables->end()) {
                                            atr = tmp;
                                            strm = &new_args[*atr].content;
                                        } else {
                                            strm->push_back(c);
                                        }
                                    } else if (c.content == "(") {
                                        strm->push_back(c);
                                        ++level;
                                    } else if (c.content == ")") {
                                        --level;
                                        if (level) strm->push_back(c);
                                    } else {
                                        strm->push_back(c);
                                    }
                                }
                                deactivated.insert(iter->first);
                                preproc_line(tokenizer_from_stream(iter->second.content), 
                                    token_stream, new_args, deactivated, cond);
                                deactivated.erase(iter->first);
                            } else {
                                token_stream.push_back(t);
                                token_stream.push_back(c);
                            }
                        } else {
                            deactivated.insert(t.content);
                            preproc_line(tokenizer_from_stream(iter->second.content), token_stream, {}, deactivated, cond);
                            deactivated.erase(t.content);
                        }
                    } else {
                        token_stream.push_back(t);
                    }
                } else {
                    token_stream.push_back(t);
                }
            }  else {
                token_stream.push_back(t);
            }
        }
        t = next_token();
    }
}

void StupidPreprocessor::append_includes(std::span<const std::filesystem::path> paths) {
    _includes.insert(_includes.end(), paths.begin(), paths.end());
}
void StupidPreprocessor::append_includes(const std::filesystem::path &path) {
    _includes.push_back(path);
}

void StupidPreprocessor::define_symbol(std::string symbol, std::string value) {
    std::vector<Token> stream;
    auto next_symbol = tokenizer_from_string(value);
    auto s = next_symbol();
    auto ctx = _context[symbol];
    while (s.type != TokenType::eof) {
        s = next_symbol();
        ctx.content.push_back(s);
    }    
}
void StupidPreprocessor::undef_symbol(const std::string &symbol) {
    _context.erase(symbol);
}

std::string StupidPreprocessor::run(const std::filesystem::path &workdir, const std::filesystem::path &src_file) {
    std::ostringstream out;
    std::ifstream indata(src_file);
    if (!indata) return {};
    run(workdir, indata, ScanMode::copy, out);;
    return std::move(out).str();

}