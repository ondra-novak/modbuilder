#include <format>
#include <filesystem>

template <>
struct std::formatter<std::filesystem::path> : std::formatter<std::string> {
    auto format(const std::filesystem::path& p, auto& ctx) const {
        // převedeme path na string v nativním formátu (např. s backslash na Windows)
        return std::formatter<std::string>::format(p.string(), ctx);
    }
};