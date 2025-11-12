#include <format>
#include <vector>
#include <functional>

struct Log {

    enum class Level {
        error = 0,      //all errors
        warning = 1,    //serious issues
        verbose = 2,    //verbose commands, describe steps
        debug = 3       //all debug informations
    };

    static constexpr std::array<std::string_view, 4> strLevel = {
        "!ERR","WARN","","debug"
    };


    using Buffer = std::vector<char>;
    using Formatter = std::function<void(Level, Buffer &)>;
    using Publisher = std::function<void(Level, const Buffer &)>;



    template<typename ... Args>
    static void debug(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::debug, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void error(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::error, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void verbose(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::verbose, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void warning(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::warning, std::move(fmt), std::forward<Args>(args)...);
        
    }

    static bool is_level_enabled(Level level) {
        return (disabled_level >= level);
    }

    template<typename ... Args>
    static void output(Level level, std::format_string<Args...> fmt, Args && ... args) {
        if (!is_level_enabled(level)) return;
        auto buf = get_buffer(level);        
        std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
        send_buffer(level, buf);
    }

    static Level disabled_level; 
    static Buffer &get_buffer(Level level);
    static void send_buffer(Level level, Buffer &buffer);

    static Formatter pre_format;
    static Formatter post_format;
    static Publisher publisher;


    static void set_formatter(Formatter preformat,
                              Formatter postformat);
    static void set_publisher(Publisher publisher);
    
    static void set_level(Level l) {
        disabled_level = l;
    }
 

};

template <std::invocable F>
struct std::formatter<F> : std::formatter<std::invoke_result_t<F>> {
    // Dědíme formatter pro návratový typ funkce
    using base = std::formatter<std::invoke_result_t<F>>;
    using result_type = std::invoke_result_t<F>;

    // parse() přebíráme beze změny
    constexpr auto parse(std::format_parse_context& ctx) {
        return base::parse(ctx);
    }

    // ve formátovací fázi funkci zavoláme a zformátujeme výsledek
    template <typename FormatContext>
    auto format(const F& f, FormatContext& ctx) const {
        return base::format(std::invoke(f), ctx);
    }
};