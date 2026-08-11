#pragma once

#include <string>
#include <string_view>

inline bool contains(const std::string& haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string::npos;
}

inline bool comesBefore(const std::string& s,
                        std::string_view earlier,
                        std::string_view later)
{
    auto e = s.find(earlier);
    auto l = s.find(later);
    return e != std::string::npos && l != std::string::npos && e < l;
}
