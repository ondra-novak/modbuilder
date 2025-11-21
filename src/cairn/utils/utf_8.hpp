#pragma once

#include <iostream>
#include <string>

void output_u8string(std::ostream &out, std::u8string_view text) {
    for (auto &x: text) out.put(x);
}

std::iostream &operator<<(std::iostream &out, std::u8string_view x) {output_u8string(out,x); return out;}
std::ostream &operator<<(std::ostream &out, std::u8string_view x) {output_u8string(out,x); return out;}
std::iostream &operator<<(std::iostream &out, const std::u8string &x) {output_u8string(out,x); return out;}
std::ostream &operator<<(std::ostream &out, const std::u8string &x) {output_u8string(out,x); return out;}

