module cairn.utils.log;

import <iostream>;
import <mutex>;
import <functional>;
import <vector>;

static std::mutex logmx;

static void pre_format_default(Log::Level lev, Log::Buffer &buff) {
    auto lvstr= Log::strLevel[static_cast<int>(lev)];
    if (!lvstr.empty()) {
        buff.insert(buff.end(), lvstr.begin(), lvstr.end());
        buff.push_back(' ');
    }
}

static void publisher_default(Log::Level , const Log::Buffer &buff){
    std::lock_guard _(logmx);
    std::cerr << std::string_view(buff.begin(), buff.end()) << std::endl;
}

Log::Level Log::disabled_level = Log::Level::verbose;
Log::Formatter Log::pre_format = pre_format_default;
Log::Formatter Log::post_format = {};
Log::Publisher Log::publisher = publisher_default;


Log::Buffer &Log::get_buffer(Level level) { 
    static thread_local Buffer buff;
    buff.clear();
    if (pre_format) pre_format(level, buff);
    return buff;
}

void Log::send_buffer(Level level, Buffer &buffer) {
    if (post_format) post_format(level, buffer);
    publisher(level, buffer);    
}

void Log::set_formatter(Formatter preformat, Formatter postformat) {
    pre_format = std::move(preformat);
    post_format = std::move(postformat);
}

void Log::set_publisher(Publisher pub) {
    publisher = std::move(pub);
}
