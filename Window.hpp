
#pragma once

#include <EGL/egl.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <utility>

#include "AppState.hpp"
#include "utils.hpp"


#define USE_EGL

#ifdef USE_EGL

class Window {
public:
    Window(int width, int height, std::string const& name)
        : width_(width), height_(height), name_(name)
    {


        eglDisp_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (!eglDisp_) throw std::runtime_error("EGL failed to get display.");
        eglInitialize(eglDisp_, &major_, &minor_);

        int nConfigs;
        eglChooseConfig(eglDisp_, eglConfigAttrs_, &eglConfig_, 1, &nConfigs);
        assert(nConfigs == 1 && "EGL returned more than one config.");

        const EGLint pbufAttrs[] = {
            EGL_WIDTH,  width,
            EGL_HEIGHT, height,
            EGL_NONE,
        };
        eglSurf_ = eglCreatePbufferSurface(eglDisp_, eglConfig_, pbufAttrs);
        if (!eglSurf_) throw std::runtime_error("EGL failed to create surface.");

        eglBindAPI(EGL_OPENGL_API);

        eglCtx_ =
            eglCreateContext(eglDisp_, eglConfig_, EGL_NO_CONTEXT, nullptr);
        if (!eglCtx_) throw std::runtime_error("EGL failed to create context.");

        eglMakeCurrent(eglDisp_, eglSurf_, eglSurf_, eglCtx_);

        GLenum ret = glewInit();
        if (ret != GLEW_OK) {
            std::cerr << glewGetErrorString(ret) << std::endl;;
            throw std::runtime_error("GLEW init failed.");
        }
    }

    ~Window()
    {
        eglTerminate(eglDisp_);
    }

    Window(const Window& o) = delete;
    Window& operator=(const Window& o) = delete;

    int width() const { return width_; }

    int height() const { return height_; }

    float aspect_ratio() const
    {
        return static_cast<float>(width_) / static_cast<float>(height_);
    }

    const std::string& name() const { return name_; }

    std::pair<int, int> frameBufferShape() { return {width_, height_}; }

    bool shouldClose() { return false; }

    void swapBuffers() {}

    void processInput() {}

private:
    int width_, height_;
    std::string const& name_;

    EGLDisplay eglDisp_;
    EGLint major_, minor_;
    EGLConfig eglConfig_;
    EGLSurface eglSurf_;
    EGLContext eglCtx_;

    // clang-format off
    static const EGLint eglConfigAttrs_[] = {
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE,       8,
        EGL_GREEN_SIZE,      8,
        EGL_RED_SIZE,        8,
        EGL_DEPTH_SIZE,      8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE,
    };
    // clang-format on
};

#endif  // #ifdef USE_EGL


#ifndef USE_EGL

class Window {
public:
    Window(int width, int height, const std::string& name)
        : width_(width), height_(height), name_(name)
    {
        glfwSetErrorCallback(Window::errorCallback);

        if (glfwInit() == 0)
            throw std::runtime_error("GLFW failed to initialize.");

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, false);

        window_ =
            glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

        if (window_ == nullptr)
            throw std::runtime_error("GLFW failed to create window.");

        glfwSetKeyCallback(window_, keyCallback_);
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);
    }

    ~Window()
    {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    Window(const Window& o) = delete;
    Window& operator=(const Window& o) = delete;

    GLFWwindow* get() { return window_; }

    int width() const { return width_; }

    int height() const { return height_; }

    float aspect_ratio() const
    {
        return static_cast<float>(width_) / static_cast<float>(height_);
    }

    bool visible() const { return visible_; }

    const std::string& name() const { return name_; }

    std::pair<int, int> frameBufferShape()
    {
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        return {w, h};
    }

    bool shouldClose() { return glfwWindowShouldClose(window_); }

    void swapBuffers() { glfwSwapBuffers(window_); }

    void processInput()
    {
        glfwPollEvents();
        AppState& s = AppState::get();
        s.up = glm::vec3(glm::row(s.M_rot, 1));

        // normalized projection of the intrinsic x axis
        // onto the extrinsic xz plane.
        // This has a problem: when the camera looks straight up or down,
        // up and front will be parallel, resulting in a cross product with
        // zero magnitude, getting the camera stuck.
        s.right = glm::cross(s.front, s.up);
        glm::vec3 right_ = glm::normalize(s.right);

        if (keyDown(GLFW_KEY_UP)) s.fov -= 1;
        if (keyDown(GLFW_KEY_DOWN)) s.fov += 1;

        s.fov = std::min(s.max_fov, s.fov);
        s.fov = std::max(10.0f, s.fov);

        float rot_a = 1.2f - (s.max_fov - s.fov) / s.max_fov;

        if (keyDown(GLFW_KEY_LEFT))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), s.front);
        if (keyDown(GLFW_KEY_RIGHT))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), s.front);
        if (keyDown(GLFW_KEY_W))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), right_);
        if (keyDown(GLFW_KEY_S))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), right_);
        if (keyDown(GLFW_KEY_A))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), s.up);
        if (keyDown(GLFW_KEY_D))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), s.up);
    }

private:
    GLFWwindow* window_;
    int width_, height_;
    std::string name_;
    bool visible_;

    bool keyDown(int key) { return glfwGetKey(window_, key) == GLFW_PRESS; }

    static void errorCallback(int err, const char* msg)
    {
        LOG("GLFW error: ", err);
        LOG(msg);
    }

    static void keyCallback_(GLFWwindow* window, int key, int scancode,
                             int action, int mods)
    {
        AppState& s = AppState::get();
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_H) s.drawUI = !s.drawUI;
        }
    }
};

#endif  // #ifndef USE_EGL
