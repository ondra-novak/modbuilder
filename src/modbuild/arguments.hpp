#pragma once
#include <string>
#include <string_view>

#ifdef _WIN32 
using ArgumentStringView = std::wstring_view;
using ArgumentString = std::wstring;
#else
using ArgumentStringView = std:u8string_view;
using ArgumentString = std::u8string;
#endif
