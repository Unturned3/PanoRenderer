
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "AppState.hpp"
#include "GUI.hpp"
#include "Image.hpp"
#include "IndexBuffer.hpp"
#include "Pose.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Window.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "utils.hpp"

static void glErrorCallback_(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length, const GLchar* msg,
                             const void* userParam)
{
    LOG("OpenGL error:");
    LOG(msg);
}

int main(int argc, char** argv)
{
#ifdef USE_EGL
    HeadlessGLContext window(640, 480, "OpenGL Test");
#else
    InteractiveGLContext window(640, 480, "OpenGL Test");
#endif

    std::string filePath = argc < 2 ? "../images/p1.jpg" : argv[1];
    Image img(filePath);

    if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug)
        glDebugMessageCallback(glErrorCallback_, nullptr);
    else
        LOG("glDebugMessageCallback not available.");

#ifndef USE_EGL
    GUI gui(window);
#endif

    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width(), img.height(), 0, GL_RGB,
                 GL_UNSIGNED_BYTE, img.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    // Rectangle (two triangles) covering the screen.
    constexpr int stride = 3;
    // clang-format off
    std::vector<float> vertices {
        -1, -1,  0,
         1,  1,  0,
        -1,  1,  0,
        -1, -1,  0,
         1, -1,  0,
         1,  1,  0,
    };
    // clang-format on

    VertexBuffer vb(vertices.data(),
                    static_cast<uint>(vertices.size() * sizeof(float)));

    Shader shader(utils::path("shaders/vertex.glsl"),
                  utils::path("shaders/frag.glsl"));
    shader.use();
    {
        // proportion of the missing sphere that's below the horizon
        double m = 1;
        if (argc >= 3) m = std::stod(argv[2]);
        double u = (double)img.width() / 2 / (double)img.height();
        float v_n = (float)(u * M_1_PI);
        float v_b = (float)(u / 2 + (1 - u) * m);
        shader.setFloat("v_norm", v_n);
        shader.setFloat("v_bias", v_b);
    }

    uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                          (void*)0);
    vb.bind();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    while (!window.shouldClose()) {
        AppState& s = AppState::get();

        updatePose();

#ifndef USE_EGL
        window.handleKeyDown();
#endif

        // Recalculate LoD, perspective, & view.
        float fov_thresh = 75.0f;
        if (s.fov <= fov_thresh)
            shader.setFloat("lod", 0.0f);
        else
            shader.setFloat(
                "lod", (s.fov - fov_thresh) / (s.max_fov - fov_thresh) + 1.0f);

        glm::mat4 M_proj = glm::perspective(glm::radians(s.fov),
                                            window.aspectRatio(), 0.1f, 2.0f);
        shader.setMat4("proj", M_proj);

        glm::mat4 M_view =
            glm::lookAt(glm::vec3(0), -glm::vec3(glm::column(s.M_rot, 2)),
                        glm::vec3(glm::column(s.M_rot, 1)));
        shader.setMat4("view", M_view);

        // Clear frame
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw projected panorama
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<int>(vertices.size() / stride));

#ifdef USE_EGL
        auto [w, h] = window.framebufferShape();
        Image frame(w, h, 3);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame.data());
        std::string name = "out.jpg";
        frame.write(name);
        LOG("frame saved to " + name);
        break;
#else
        if (s.drawUI) {
            gui.update();
            gui.render();
        }
        window.swapBuffers();
#endif
    }

    return 0;
}
