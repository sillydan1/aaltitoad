#ifndef AALTITOAD_PARSE_ERROR_H
#define AALTITOAD_PARSE_ERROR_H
#include <aaltitoadpch.h>

struct parse_error : public std::logic_error {
    explicit parse_error(const std::string& msg) : std::logic_error(msg.c_str()) {}
};

#endif //AALTITOAD_PARSE_ERROR_H
