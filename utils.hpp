
#pragma once
#include "config.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

typedef unsigned int uint;

#ifdef LOG_FILE_AND_LINE
#define LOG(...) utils::log(__FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG(...) utils::log(__VA_ARGS__)
#endif

namespace utils {

// Variadic Templates
// https://stackoverflow.com/a/29326784
template <typename... Args>
void log(Args&&... args)
{
    (std::cout << ... << args) << std::endl;
}

std::string read_file(const std::string& path)
{
    std::ifstream f { path };
    if (f.fail())
        throw std::runtime_error("Failed to read file " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string path(const std::string& p)
{
    return root_dir / std::filesystem::path(p);
}

} // namespace utils
