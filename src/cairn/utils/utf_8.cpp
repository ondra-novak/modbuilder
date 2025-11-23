export module cairn.utils.utf8;

import <iostream>;
import <string>;
import <algorithm>;

export inline void output_u8string(std::ostream &out, std::u8string_view text) {
    for (auto &x: text) out.put(x);
}

export inline std::iostream &operator<<(std::iostream &out, std::u8string_view x) {output_u8string(out,x); return out;}
export inline std::ostream &operator<<(std::ostream &out, std::u8string_view x) {output_u8string(out,x); return out;}
export inline std::iostream &operator<<(std::iostream &out, const std::u8string &x) {output_u8string(out,x); return out;}
export inline std::ostream &operator<<(std::ostream &out, const std::u8string &x) {output_u8string(out,x); return out;}

export inline std::u8string_view u8_from_string(std::string_view s) {
    return std::u8string_view(reinterpret_cast<const char8_t *>(s.data()),s.size());
}
export inline std::string_view string_from_u8(std::u8string_view s) {
    return std::string_view(reinterpret_cast<const char *>(s.data()),s.size());
}


constexpr  std::size_t REPLACEMENT = 0xFFFD;

export template<typename CharType, typename Iter, std::output_iterator<CharType> OutIter>
inline OutIter from_utf8(Iter beg, Iter end, OutIter out) {
    static_assert(std::is_integral_v<CharType>);
    if constexpr(sizeof(CharType) == 1) {
        return std::transform(beg, end, out, [](auto &x){return static_cast<CharType>(x);});        
    } else {        
        auto push_codepoint = [&](std::size_t cp) {
            if constexpr (sizeof(CharType) >= 4) {
                *out++ = (static_cast<CharType>(cp));
            } else {
                if (cp <= 0xFFFF) {
                    *out++ = static_cast<CharType>(cp);
                } else {
                    // encode as UTF-16 surrogate pair
                    cp -= 0x10000;
                    wchar_t high = static_cast<CharType>((cp >> 10) + 0xD800);
                    wchar_t low  = static_cast<CharType>((cp & 0x3FF) + 0xDC00);
                    *out++= high;
                    *out++= low;
                }
            }
        };

        int len = 0;
        std::size_t cp;
        for (auto &x: std::ranges::subrange(beg, end)) {
            unsigned char b = static_cast<unsigned char>(x);
            if (len) {
                if (b & 0x80) {
                    cp = (cp << 6) | (b & 0x3F);
                    --len;
                    if (len == 0) push_codepoint(cp);
                    continue;
                } else {
                    push_codepoint(REPLACEMENT);
                }
            }
            if (!(b & 0x80)) {
                push_codepoint(b);
            } else if ((b & 0xE0) == 0xC0) {
                cp = b & 0x1F;
                len = 1;
            } else if ((b & 0xF0) == 0xE0) {
                cp = b & 0x0F;
                len = 2;
            } else if ((b & 0xF8) == 0xF0) {
                cp = b & 0x07;
                len = 3;
            } else {
                push_codepoint(REPLACEMENT);
            }
        }
        return out;
    }
}

export std::size_t decodeUtf16UnknownOrder(std::size_t a, std::size_t b)
{
    bool aHigh = (a >= 0xD800 && a <= 0xDBFF);
    bool aLow  = (a >= 0xDC00 && a <= 0xDFFF);
    bool bHigh = (b >= 0xD800 && b <= 0xDBFF);
    bool bLow  = (b >= 0xDC00 && b <= 0xDFFF);

    // Jediná validní kombinace je: high + low
    if (aHigh && bLow) {
        size_t highPart = a - 0xD800;
        size_t lowPart  = b - 0xDC00;
        return static_cast<char32_t>((highPart << 10) + lowPart + 0x10000);
    }

    if (bHigh && aLow) { // vyměněné pořadí → podporujeme
        size_t highPart = b - 0xD800;
        size_t lowPart  = a - 0xDC00;
        return static_cast<char32_t>((highPart << 10) + lowPart + 0x10000);
    }

    return REPLACEMENT;
}

export template<typename Iter, std::output_iterator<char8_t> OutIter>
inline OutIter to_utf8(Iter beg, Iter end, OutIter out) {
    using CharType = std::remove_cvref_t<decltype(*beg)>;
    static_assert(std::is_integral_v<CharType>);
    if constexpr(sizeof(CharType) == 1) {
        return std::transform(beg, end, out, [](auto &x){return static_cast<char8_t>(x);});
    } else {
        std::size_t surg = 0;
            for (auto &x: std::ranges::subrange(beg, end)) {
            std::size_t b = static_cast<std::size_t>(x);
            if (b >= 0xD800 && b <= 0xDFFF) {
                if (surg == 0) {
                    surg = b;
                    continue;
                } else {
                    b = decodeUtf16UnknownOrder(surg, b);
                    surg = 0;
                }
            }
            if (b < 0x80) {
                *out++ = static_cast<char8_t>(b);
            } else if (b < 0x800) {
                *out++ = static_cast<char8_t>((b >> 6)   | 0xC0);
                *out++ = static_cast<char8_t>((b & 0x3F) | 0x80);
            } else if (b < 0x10000) {
                *out++ = static_cast<char8_t>((b >> 12)         | 0xE0);
                *out++ = static_cast<char8_t>(((b >> 6) & 0x3F) | 0x80);
                *out++ = static_cast<char8_t>(( b       & 0x3F) | 0x80);
            } else {
                *out++ = static_cast<char8_t>((b >> 18)          | 0xF0);
                *out++ = static_cast<char8_t>(((b >> 12) & 0x3F) | 0x80);
                *out++ = static_cast<char8_t>(((b >> 8 ) & 0x3F) | 0x80);
                *out++ = static_cast<char8_t>((b         & 0x3F) | 0x80);
            }
        }
        return out;
    }
}

