
#pragma once
#include "utils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

static uint compile_shader(uint type, const std::string& path);

static uint create_shader(
    const std::string vertShaderPath, const std::string fragShaderPath);

class Shader {
public:
    Shader(const std::string& vertShaderPath, const std::string& fragShaderPath)
    {
        id_ = create_shader(vertShaderPath, fragShaderPath);
    }

    void use() const { glUseProgram(id_); }

    void setBool(const std::string& name, bool val) const
    {
        glUniform1i(getULoc(name), static_cast<int>(val));
    }

    void setInt(const std::string& name, int val) const
    {
        glUniform1i(getULoc(name), val);
    }

    void setFloat(const std::string& name, float val) const
    {
        glUniform1f(getULoc(name), val);
    }

    void setVec2(const std::string& name, const glm::vec2& v) const
    {
        glUniform2f(getULoc(name), v.x, v.y);
    }

    void setVec3(const std::string& name, const glm::vec3& v) const
    {
        glUniform3f(getULoc(name), v.x, v.y, v.z);
    }

    void setMat4(const std::string& name, const glm::mat4& m) const
    {
        glUniformMatrix4fv(getULoc(name), 1, GL_FALSE, glm::value_ptr(m));
    }

    const uint& id() const { return id_; }

private:
    uint id_;
    int getULoc(const std::string& name) const
    {
        return glGetUniformLocation(id_, name.c_str());
    }
};

static uint compile_shader(uint type, const std::string& path)
{
    const std::string& src = utils::read_file(path);
    if (src.length() <= 0)
        throw std::runtime_error("Shader file " + path + " is empty!");

    uint id = glCreateShader(type);
    const char* s = src.c_str();
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

static uint create_shader(
    const std::string vertShaderPath, const std::string fragShaderPath)
{
    uint p = glCreateProgram();
    uint vs = compile_shader(GL_VERTEX_SHADER, vertShaderPath);
    uint fs = compile_shader(GL_FRAGMENT_SHADER, fragShaderPath);

    if (!vs || !fs) {
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
