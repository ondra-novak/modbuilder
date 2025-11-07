#include <format>
#include <span>

struct Log {

    enum class Level {
        error = 0,
        warning = 1,
        debug = 2
    };

    template<typename ... Args>
    static void debug(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::debug, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void error(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::error, std::move(fmt), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void warning(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::warning, std::move(fmt), std::forward<Args>(args)...);
        
    }

    template<typename ... Args>
    static void output(Level level, std::format_string<Args...> fmt, Args && ... args) {
        if (disabled_level >= level) return;
        auto buf = get_buffer();        
        std::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
        send_buffer(level, buf);
    }

    static Level disabled_level; 
    static std::vector<char> get_buffer();
    static void send_buffer(Level level, std::span<const char> buffer);

};
