#ifndef WOOPS_UTIL_TYPEDEF_H_
#define WOOPS_UTIL_TYPEDEF_H_

#include <string>


namespace woops
{
using Hostid = int32_t;
using Tableid = int32_t; 
using ParamIndex = int32_t;
using Iteration = int32_t;
using Byte = char;
using Bytes = std::string;

#if __has_cpp_attribute(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#elif __has_cpp_attribute(gnu::unused)
#define MAYBE_UNUSED [[gnu::unused]]
#else
#define MAYBE_UNUSED
#endif

#ifdef DEBUG
MAYBE_UNUSED static std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

MAYBE_UNUSED static std::string chars_to_hex(const char* input, size_t len)
{
    static const char* const lut = "0123456789ABCDEF";

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}
#endif
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_TYPEDEF_H_ */
