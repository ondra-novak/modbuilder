#include "log.hpp"
#include <iostream>
#include <mutex>

static std::mutex logmx;

Log::Level Log::disabled_level = Log::Level::verbose;
Log::Formatter Log::pre_format = [](Log::Level lev, Log::Buffer &buff) {
     using namespace std::chrono;
/*    auto now = system_clock::now();
    auto secs = floor<seconds>(now);
    auto ms = duration_cast<milliseconds>(now - secs).count();
    std::time_t t = system_clock::to_time_t(secs);
    std::tm utc_time = *std::gmtime(&t);
    buff.resize(30);
    buff.resize(std::strftime(buff.data(), buff.size(), "%Y-%m-%d %H:%M:%S", &utc_time));
    std::format_to(std::back_inserter(buff),".{:03d}Z {} ",  static_cast<int>(ms), strLevel[static_cast<int>(lev)]);
    */
    auto lvstr= strLevel[static_cast<int>(lev)];
    if (!lvstr.empty()) {
        buff.insert(buff.end(), lvstr.begin(), lvstr.end());
        buff.push_back(' ');
    }
       
};
Log::Formatter Log::post_format = {};
Log::Publisher Log::publisher = [](auto, const Log::Buffer &buff){
    std::lock_guard _(logmx);
    std::cerr << std::string_view(buff.begin(), buff.end()) << std::endl;
};


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
