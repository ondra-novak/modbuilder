#include "arguments.hpp"
#include <exception>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>      // std::getenv
#include <optional>

#if defined(_WIN32)
    const char PATH_SEPARATOR = ';';
#else
    const char PATH_SEPARATOR = ':';
#endif

// ---------------------------------------------------------------
// Vrátí vektor cest z proměnné PATH (bez prázdných položek)
inline std::vector<std::filesystem::path> get_path_entries()
{
    std::vector<std::filesystem::path> result;

    const char* path_env = std::getenv("PATH");
    if (!path_env) return result;               // PATH není nastaven

    std::string path_str = path_env;
    std::size_t start = 0;

    while (start < path_str.size()) {
        std::size_t end = path_str.find(PATH_SEPARATOR, start);
        if (end == std::string::npos) end = path_str.size();

        std::string entry = path_str.substr(start, end - start);
        if (!entry.empty()) {
            result.emplace_back(entry);
        }
        start = end + 1;
    }
    return result;
}

// ---------------------------------------------------------------
// Najde první existující soubor s daným jménem v PATH
inline std::filesystem::path find_in_path(const std::filesystem::path & filename)
{
    if (filename.filename() == filename) {

        for (const auto& dir : get_path_entries()) {
            std::filesystem::path candidate = (dir / filename).lexically_normal();
            if (std::filesystem::exists(candidate) &&
                std::filesystem::is_regular_file(candidate)) {
                return candidate;
            }
        }
    }
    return (std::filesystem::current_path()/filename).lexically_normal();
}
