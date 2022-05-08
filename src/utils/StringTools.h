#pragma once

#include "logger.h"
#include "utils.h"
#include <memory>
#include <string>
#include <vector>
#include <wut_types.h>

template<typename... Args>
std::string string_format(const std::string &format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    auto size  = static_cast<size_t>(size_s);
    auto buf   = make_unique_nothrow<char[]>(size);
    if (!buf) {
        DEBUG_FUNCTION_LINE_ERR("string_format failed, not enough memory");
        OSFatal("string_format failed, not enough memory");
        return std::string("");
    }
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

class StringTools {
public:
    static uint32_t hash(char *str);
};