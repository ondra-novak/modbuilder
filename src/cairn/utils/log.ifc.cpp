
export module cairn.utils.log;

import <format>;
import <vector>;
import <functional>;
import <array>;
import <type_traits>;


template<typename T>
concept not_void = !std::is_void_v<T>;

export template<typename T>
concept has_log_format_function = requires (T v) {
    {log_format(v)}->not_void;
};

export struct Log {

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

    template<typename T>
    struct ConvertType {
        using type =  T;
    };

    template<std::invocable T>
    struct ConvertType<T> {
        using type =  std::invoke_result_t<T>;
    };

    template<typename Arg>
    using Convert = typename ConvertType<Arg>::type;




    template<typename ... Args>
    static void debug(std::format_string<Convert<Args>...> fmt, Args && ... args) {
        output(Level::debug, fmt.get(), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void error(std::format_string<Convert<Args>...> fmt, Args && ... args) {
        output(Level::error, fmt.get(), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void verbose(std::format_string<Convert<Args>...> fmt, Args && ... args) {
        output(Level::verbose, fmt.get(), std::forward<Args>(args)...);
    }

    template<typename ... Args>
    static void warning(std::format_string<Convert<Args>...> fmt, Args && ... args) {
        output(Level::warning, fmt.get(), std::forward<Args>(args)...);
        
    }

    static bool is_level_enabled(Level level) {
        return (disabled_level >= level);
    }

    template<typename Arg>
    static auto lazy_evaluate(Arg &&arg) {
        if constexpr(std::is_invocable_v<Arg>) {
            return lazy_evaluate(arg());
        } else if constexpr(has_log_format_function<Arg>) {
            return lazy_evaluate(log_format(std::forward<Arg>(arg)));     
        } else {
            return arg;
        }
    }


    template<typename ... Args>
    static void output(Level level, std::string_view fmt, Args && ... args) {
        if (!is_level_enabled(level)) return;
        auto buf = get_buffer(level);
        auto tup = std::make_tuple(lazy_evaluate(args)...);
        std::apply([&](auto &...args) {
            std::vformat_to(std::back_inserter(buf), fmt,  std::make_format_args(args...));
        }, tup);

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


