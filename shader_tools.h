
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "utils.h"

static uint compileShader(uint type, const std::string& src) {
    uint id = glCreateShader(type);
    const char *s = src.c_str();
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);

    int res = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &res);
    if (res == GL_FALSE) {
        LOG("Shader failed to compile!");
        return 0;
    }
    LOG("Create shader OK");
    return id;
}

uint createShader(const std::string& vertexShader,
                  const std::string& fragmentShader) {
    uint p = glCreateProgram();
    uint vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    uint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glValidateProgram(p);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return p;
}
