
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <utility>

#include "AppState.hpp"
#include "Image.hpp"
#include "utils.hpp"

class GLContext {
public:
    GLContext(int width, int height, const std::string& name)
        : width_(width), height_(height), name_(name)
    {
    }

    ~GLContext() = default;

    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;

    GLContext(GLContext&&) = delete;
    GLContext& operator=(GLContext&&) = delete;

    int width() const { return width_; }

    int height() const { return height_; }

    float aspectRatio() const
    {
        return static_cast<float>(width_) / static_cast<float>(height_);
    }

    const std::string& name() const { return name_; }

    virtual std::pair<int, int> framebufferShape() = 0;

    virtual bool shouldClose() = 0;

protected:
    int width_, height_;
    std::string name_;
};

#ifdef USE_EGL

#include <EGL/egl.h>

class HeadlessGLContext : public GLContext {
public:
    HeadlessGLContext(int width, int height, std::string const& name)
        : GLContext(width, height, name)
    {
        eglDisp_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (!eglDisp_) throw std::runtime_error("EGL failed to get display.");

        eglInitialize(eglDisp_, &major_, &minor_);

        int nConfigs;
        eglChooseConfig(eglDisp_, eglConfigAttrs_, &eglConfig_, 1, &nConfigs);
        assert(nConfigs == 1 && "EGL returned more than one config.");

        const EGLint pbufAttrs[] = {
            EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE,
        };

        eglSurf_ = eglCreatePbufferSurface(eglDisp_, eglConfig_, pbufAttrs);
        if (!eglSurf_)
            throw std::runtime_error("EGL failed to create surface.");

        eglBindAPI(EGL_OPENGL_API);

        eglCtx_ =
            eglCreateContext(eglDisp_, eglConfig_, EGL_NO_CONTEXT, nullptr);
        if (!eglCtx_) throw std::runtime_error("EGL failed to create context.");

        eglMakeCurrent(eglDisp_, eglSurf_, eglSurf_, eglCtx_);

        GLenum ret = glewInit();
        if (ret != GLEW_OK) {
            std::cerr << glewGetErrorString(ret) << std::endl;
            throw std::runtime_error("GLEW init failed.");
        }
    }

    ~HeadlessGLContext() { eglTerminate(eglDisp_); }

    std::pair<int, int> framebufferShape() override
    {
        return {width_, height_};
    }

    bool shouldClose() override { return false; }

private:
    EGLDisplay eglDisp_;
    EGLint major_, minor_;
    EGLConfig eglConfig_;
    EGLSurface eglSurf_;
    EGLContext eglCtx_;

    // clang-format off
    static constexpr EGLint eglConfigAttrs_[] = {
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

class InteractiveGLContext : public GLContext {
public:
    InteractiveGLContext(int width, int height, const std::string& name)
        : GLContext(width, height, name)
    {
        glfwSetErrorCallback(InteractiveGLContext::errorCallback);

        if (glfwInit() == 0)
            throw std::runtime_error("GLFW failed to initialize.");

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_VISIBLE, false);

        window_ =
            glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

        if (window_ == nullptr)
            throw std::runtime_error("GLFW failed to create window.");

        glfwSetWindowUserPointer(window_, this);
        glfwSetKeyCallback(window_, InteractiveGLContext::keyCallback);
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        GLenum ret = glewInit();
        if (ret != GLEW_OK) {
            std::cerr << glewGetErrorString(ret) << std::endl;
            throw std::runtime_error("GLEW init failed.");
        }
    }

    ~InteractiveGLContext()
    {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    GLFWwindow* get() { return window_; }

    bool visible() const { return visible_; }

    void swapBuffers() { glfwSwapBuffers(window_); }

    std::pair<int, int> framebufferShape() override
    {
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        return {w, h};
    }

    bool shouldClose() override { return glfwWindowShouldClose(window_); }

    void handleKeyDown()
    {
        glfwPollEvents();
        AppState& s = AppState::get();

        if (s.enable_trajectory)
            return;  // Disable manual control when auto trajectory is enabled

        s.up = glm::vec3(glm::row(s.M_rot, 1));

        // normalized projection of the intrinsic x axis
        // onto the extrinsic xz plane.
        // This has a problem: when the camera looks straight up or down,
        // up and front will be parallel, resulting in a cross product with
        // zero magnitude, getting the camera stuck.
        s.right = glm::cross(s.front, s.up);
        glm::vec3 right_ = glm::normalize(s.right);

        if (keyDown(GLFW_KEY_UP)) s.hfov -= 1;
        if (keyDown(GLFW_KEY_DOWN)) s.hfov += 1;

        s.hfov = std::min(s.max_fov, s.hfov);
        s.hfov = std::max(10.0f, s.hfov);

        float rot_a = 1.2f - (s.max_fov - s.hfov) / s.max_fov;

        if (keyDown(GLFW_KEY_LEFT))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), s.front);
        if (keyDown(GLFW_KEY_RIGHT))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), s.front);
        /*  NOTE: we are rendering images upside down. Technically we should
            swap the behavior of the W/S keys, but since we're not going to
            use these keys to generate videos, this is left as it is. */
        if (keyDown(GLFW_KEY_W))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), right_);
        if (keyDown(GLFW_KEY_S))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), right_);
        if (keyDown(GLFW_KEY_A))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(rot_a), s.up);
        if (keyDown(GLFW_KEY_D))
            s.M_rot = glm::rotate(s.M_rot, glm::radians(-rot_a), s.up);
    }

    void handleKeyPress(int key, int scancode, int action, int mods)
    {
        AppState& s = AppState::get();
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_H) s.drawUI = !s.drawUI;
            if (key == GLFW_KEY_T) {
                auto [w, h] = framebufferShape();
                Image frame(w, h, 3);
                glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE,
                             frame.data());
                std::string name = "out.jpg";
                frame.write(name);
                LOG("frame saved to " + name);
            }
            if (s.poses.has_value()) {
                if (key == GLFW_KEY_J) {
                    s.pose_idx -= 1;
                    if (s.pose_idx < 0) s.pose_idx += s.poses->shape[0];
                }
                if (key == GLFW_KEY_K) {
                    s.pose_idx += 1;
                    s.pose_idx %= s.poses->shape[0];
                }
            }
        }
    }

private:
    GLFWwindow* window_;
    bool visible_;

    bool keyDown(int key) { return glfwGetKey(window_, key) == GLFW_PRESS; }

    static void errorCallback(int err, const char* msg)
    {
        LOG("GLFW error: ", err);
        LOG(msg);
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods)
    {
        auto w = static_cast<InteractiveGLContext*>(
            glfwGetWindowUserPointer(window));
        w->handleKeyPress(key, scancode, action, mods);
    }
};
