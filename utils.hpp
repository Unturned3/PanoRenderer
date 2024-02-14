
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "config.h"

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
    std::ifstream f {path};
    if (f.fail()) throw std::runtime_error("Failed to read file " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string path(const std::string& p)
{
    return root_dir / std::filesystem::path(p);
}

void probe_OpenGL_properties()
{
    LOG("OpenGL version: ", glGetString(GL_VERSION));

    LOG("NV_vdpau_interop: ", GLEW_NV_vdpau_interop);
    LOG("NV_vdpau_interop2: ", GLEW_NV_vdpau_interop2);

    int maxVertAtrbs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertAtrbs);
    LOG("Max number of vertex attributes: ", maxVertAtrbs);

    int maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    LOG("Max texture size: ", maxTexSize);

    int max3DTexSize;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTexSize);
    LOG("Max 3D texture size: ", max3DTexSize);

    int maxRectTexSize;
    glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &maxRectTexSize);
    LOG("Max rectangular texture size: ", maxRectTexSize);
}

std::string pretty_matrix(const float* a, int n, int m, int sig_figs,
                          bool col_major = true)
{
    std::stringstream s;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            int k = col_major ? (j * m + i) : (i * m + j);
            s << std::fixed << std::setprecision(sig_figs)
              << std::setw(sig_figs + 4) << *(a + k);
        }
        s << "\n";
    }
    return s.str();
}

}  // namespace utils
