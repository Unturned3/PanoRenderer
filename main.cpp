
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
    Window window(1280, 720, "OpenGL Test", argc <= 3);

    std::string filePath = argc < 2 ? "../images/p1.jpg" : argv[1];
    Image img(filePath);

    if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug)
        glDebugMessageCallback(glErrorCallback_, nullptr);
    else
        LOG("glDebugMessageCallback not available.");

    GUI gui(window);

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

    // Trajectory generation parameters
    float focal_accel_m = 5;
    float pose_accel_m = 3;
    float delta = 1.0f / 60.0f;
    float max_vel = 1;

    float mean_pitch_accel = 0;
    float mean_focal_accel = 0;

    glm::vec3 prev_pose {0, 0, 75};
    glm::vec3 prev_vel {0, 0, 0};

    while (!window.shouldClose()) {
        AppState& s = AppState::get();

        gui.update();

        /*
        // Calculate trajectory update
        glm::vec2 pitch_yaw_accel {
            glm::gaussRand(mean_pitch_accel, 1.0f) / 1.5,  // pitch
            glm::gaussRand(0.0f, 1.0f),                    // yaw
        };
        glm::vec3 accel {
            glm::normalize(pitch_yaw_accel) * pose_accel_m,
            glm::gaussRand(mean_focal_accel, 1.0f) * focal_accel_m,  // focal
        };

        // Update trajectory
        glm::vec3 vel = prev_vel + accel * delta;

        if (glm::length(vel) > max_vel) vel = max_vel * glm::normalize(vel);

        glm::vec3 pose {prev_pose + vel * delta};

        mean_pitch_accel = -pose.x / 2;
        mean_focal_accel = (55 - pose.z) / 35;

        prev_pose = pose;
        prev_vel = vel;

        AppState& s = AppState::get();
        if (s.randomTrajectory) {
            {
                s.up = glm::vec3(glm::row(s.M_rot, 1));
                s.right = glm::cross(s.front, s.up);
                glm::vec3 right_ = glm::normalize(s.right);

                // M_rot = glm::rotate(M_rot, glm::radians(-rot_a), front);
                s.M_rot = glm::rotate(s.M_rot, glm::radians(vel.x), right_);
                s.M_rot = glm::rotate(s.M_rot, glm::radians(vel.y), s.up);
            }
        }
        */

        window.processInput();

        // Clear frame
        glClear(GL_COLOR_BUFFER_BIT);

        // Recalculate FoV, LoD, perspective, & view.
        float fov_thresh = 75.0f;
        if (s.fov <= fov_thresh)
            shader.setFloat("lod", 0.0f);
        else
            shader.setFloat(
                "lod", (s.fov - fov_thresh) / (s.max_fov - fov_thresh) + 1.0f);

        glm::mat4 M_proj = glm::perspective(glm::radians(s.fov),
                                            window.aspect_ratio(), 0.1f, 2.0f);
        shader.setMat4("proj", M_proj);

        glm::mat4 M_view =
            glm::lookAt(glm::vec3(0), -glm::vec3(glm::column(s.M_rot, 2)),
                        glm::vec3(glm::column(s.M_rot, 1)));
        shader.setMat4("view", M_view);

        // Draw projected panorama
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<int>(vertices.size() / stride));

        if (s.showUI) gui.render();

        window.swapBuffers();

        if (!window.visible()) {
            /*
            Swap again so both the front and back buffer is guaranteed to
            contain the latest frame, which is then read by glReadPixels
            below. If we omit this, glReadPixels may return an empty frame on
            some platforms (because it read the other buffer which was not
            rendered to yet).
            */
            window.swapBuffers();
            break;
        }
    }

    {
        auto [w, h] = window.frameBufferShape();
        LOG("Frame buffer shape: ", w, " ", h);
        Image frame(w, h, 3);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame.data());
        frame.write("out.jpg");
    }

    return 0;
}
