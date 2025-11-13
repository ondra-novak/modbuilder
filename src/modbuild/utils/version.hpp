#pragma once

#include <sstream>
#include <vector>
#include <string>

class Version {
public:
    std::vector<int> parts;

    Version() = default;
    Version(const std::string& str) {
        std::stringstream ss(str);
        std::string part;
        while (std::getline(ss, part, '.')) {
            parts.push_back(std::stoi(part));
        }
    }

    bool operator<(const Version& other) const {
        size_t n = std::max(parts.size(), other.parts.size());
        for (size_t i = 0; i < n; ++i) {
            int a = (i < parts.size()) ? parts[i] : 0;
            int b = (i < other.parts.size()) ? other.parts[i] : 0;
            if (a < b) return true;
            if (a > b) return false;
        }
        return false;
    }

    bool operator==(const Version& other) const {
        return !(*this < other) && !(other < *this);
    }

    std::string to_string() const {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) result += ".";
            result += std::to_string(parts[i]);
        }
        return result;
    }

    bool operator>(const Version& other) const { return other < *this; }
    bool operator<=(const Version& other) const { return !(other < *this); }
    bool operator>=(const Version& other) const { return !(*this < other); }
};
