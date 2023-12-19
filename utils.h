
#pragma once
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

typedef unsigned int uint;

#ifdef LOG_FILE_AND_LINE
    #define LOG(...) utils::log(__FILE__, __LINE__, __VA_ARGS__)
#else
    #define LOG(...) utils::log(__VA_ARGS__)
#endif

namespace utils {

// Variadic Templates
// https://stackoverflow.com/a/29326784
template<typename ...Args>
void log(Args && ...args)
{
    (std::cout << ... << args) << std::endl;
}

std::string read_shader_src(std::string path) {
    std::ifstream f{path};
    assert(!f.fail());
    std::stringstream ss;
    ss << f.rdbuf();
    std::string s { ss.str() };
    assert(s.length() > 0);
    return s;
}


} // namespace utils
