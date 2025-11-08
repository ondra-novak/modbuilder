#include <format>
#include <vector>
#include <span>
#include <functional>

struct Log {

    enum class Level {
        error = 0,
        warning = 1,
        debug = 2
    };

    static constexpr std::array<std::string_view, 3> strLevel = {
        "!ERR","WARN","    "
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
    static void warning(std::format_string<Args...> fmt, Args && ... args) {
        output(Level::warning, std::move(fmt), std::forward<Args>(args)...);
        
    }

    template<typename ... Args>
    static void output(Level level, std::format_string<Args...> fmt, Args && ... args) {
        if (disabled_level >= level) return;
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
