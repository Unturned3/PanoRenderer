
#pragma once
#include "utils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <exception>

class Window {

public:
    Window(int width, int height, const std::string& name, bool visible = true,
        bool resizable = false, int gl_major_ver = 4, int gl_minor_ver = 1)
        : width_(width)
        , height_(height)
        , name_(name)
    {
        glfwSetErrorCallback(Window::glfwErrorCallback_);

        if (glfwInit() == 0)
            throw std::runtime_error("GLFW failed to initialize.");

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, visible);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major_ver);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor_ver);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, resizable);

        window_
            = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

        // Check if window was created successfully
        if (window_ == nullptr)
            throw std::runtime_error("GLFW failed to create window.");

        glfwMakeContextCurrent(window_);
    }

    ~Window() { glfwTerminate(); }

    Window(const Window& o) = delete;
    Window& operator=(const Window& o) = delete;

    GLFWwindow* get() { return window_; }

    int width() const { return width_; }

    int height() const { return height_; }

    const std::string& name() const { return name_; }

private:
    GLFWwindow* window_;
    int width_, height_;
    std::string name_;

    static void glfwErrorCallback_(int err, const char* msg)
    {
        LOG("GLFW error: ", err);
        LOG(msg);
    }
};
