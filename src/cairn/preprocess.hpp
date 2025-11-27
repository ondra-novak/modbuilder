#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>


class StupidPreprocessor {
public:

    void append_includes(std::span<const std::filesystem::path> paths);
    void append_includes(const std::filesystem::path &path);

    void define_symbol(std::string symbol, std::string value);
    void undef_symbol(const std::string &symbol);

    enum class ScanMode {
        skip, //skip until #endif - enters all #if , #ifdef, #else
        collect, //collect defines, do not produce output
        copy  //copy lines to output. This preprocessor doesn't expand macros in code
    };

    enum class Command {
        _if,
        _ifdef,
        _ifndef,
        _elif,
        _elifdef,
        _elifndef,
        _else,
        _endif,
        _define,
        _undef,
        _include,        
        eof
    };

    static constexpr auto strCommand =  std::array<std::string_view, 11>({
        "#if",
        "#ifdef",
        "#ifndef",
        "#elif",
        "#elifdef",
        "#elifndef",
        "#else",
        "#endif",
        "#define",
        "#undef",
        "#include",        
    });

    struct NextCommand {
        Command cmd;
        std::string args;
    };

    NextCommand run(const std::filesystem::path &cur_dir, std::istream &in, ScanMode mode, std::ostream &out);


    std::string run(const std::filesystem::path &workdir, const std::filesystem::path &src_file);

    const auto &get_include_paths() const {return _includes;}


protected:

    enum class TokenType {
        begin,               //begin token, always returned as first, read next one
        eof,                //end of stream
        identifier,         //identifier
        number,
        xnumber,
        string,
        symbol,       //one symbol
        white         //whitespace
    };

    struct Token {
        TokenType type = TokenType::begin;
        std::string content = {};
    };



    struct MacroDef {
        std::vector<Token> content;
        std::optional<std::vector<std::string> > variables;
    };

    using IncludeList = std::vector<std::filesystem::path>;

    using MacroMap = std::unordered_map<std::string, MacroDef>;

    MacroMap _context;
    IncludeList _includes;

    static constexpr Command find(std::string_view text) {
        auto iter = std::find(strCommand.begin(), strCommand.end(), text);
        if (iter == strCommand.end()) return Command::eof;
        else return static_cast<Command>(std::distance(strCommand.begin(), iter));
    }



    void parse_define(std::string_view args);
    void parse_undef(std::string_view args);
    void parse_include(const std::filesystem::path &cur_dir, std::string_view args);
    bool parse_if(std::string_view args);
    bool parse_ifdef(std::string_view args);

    template<typename Fn>
    static auto tokenizer(Fn &&fn);


    static auto tokenizer_from_stream(std::span<const Token> stream);
    static auto tokenizer_from_string(std::string_view str);


    template<typename Fn>
    void preproc_line(Fn &&next_token, std::vector<Token> &token_stream, const MacroMap &arguments, std::unordered_set<std::string> &deactivated, bool cond) const;

    long evaluate(auto &&next_token);
    std::pair<long, Token> evaluate_oror(auto &&next_token);
    std::pair<long, Token> evaluate_andand(auto &&next_token);
    std::pair<long, Token> evaluate_or(auto &&next_token);
    std::pair<long, Token> evaluate_xor(auto &&next_token);
    std::pair<long, Token> evaluate_and(auto &&next_token);
    std::pair<long, Token> evaluate_equal(auto &&next_token);
    std::pair<long, Token> evaluate_relation(auto &&next_token);
    std::pair<long, Token> evaluate_shift(auto &&next_token);
    std::pair<long, Token> evaluate_sum(auto &&next_token);
    std::pair<long, Token> evaluate_multiply(auto &&next_token);
    std::pair<long, Token> evaluate_unar(auto &&next_token);


};
