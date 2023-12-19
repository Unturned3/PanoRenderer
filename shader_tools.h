
#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "utils.h"

static uint compile_shader(uint type, const std::string& path) {
    const std::string& src = utils::read_file(path);
    if (src.length() <= 0)
        throw std::runtime_error("Shader file " + path + " is empty!");

    uint id = glCreateShader(type);
    const char *s = src.c_str();
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[512];
        LOG(path + ":");
        glGetShaderInfoLog(id, 512, nullptr, info);
        LOG(info);
        throw std::runtime_error("Shader compilation failed.");
    }
    return id;
}

uint create_shader(const std::string vertShaderPath,
                  const std::string fragShaderPath) {
    uint p = glCreateProgram();
    uint vs = compile_shader(GL_VERTEX_SHADER, vertShaderPath);
    uint fs = compile_shader(GL_FRAGMENT_SHADER, fragShaderPath);

    if (!vs || ! fs) {
        return 0;
    }

    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glValidateProgram(p);

    int ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char info[512];
        glGetShaderInfoLog(p, 512, nullptr, info);
        LOG(info);
        throw std::runtime_error("Shader linking failed.");
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}
