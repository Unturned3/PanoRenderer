
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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init("#version 150");

    if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug)
        glDebugMessageCallback(glErrorCallback_, nullptr);
    else
        LOG("glDebugMessageCallback not available.");

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

    int frame_cnt = 0;
    float fps_sum = 0;
    float fps = 1;

    // Trajectory generation parameters
    float focal_accel_m = 5;
    float pose_accel_m = 3;
    float delta = 1.0f / 60.0f;
    float max_vel = 1;

    float mean_pitch_accel = 0;
    float mean_focal_accel = 0;

    glm::vec3 prev_pose {0, 0, 75};
    glm::vec3 prev_vel {0, 0, 0};

    while (glfwWindowShouldClose(window.get()) == 0) {
        // Compute fps
        frame_cnt++;
        fps_sum += io.Framerate;
        {
            int limit = (int)fps / 10 + 1;
            if (frame_cnt == limit) {
                fps = fps_sum / (float)limit;
                frame_cnt = 0;
                fps_sum = 0;
            }
        }

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

                /*
                fov += vel.z;
                fov = std::min(max_fov, fov);
                fov = std::max(10.0f, fov);
                */

                // M_rot = glm::rotate(M_rot, glm::radians(-rot_a), front);
                s.M_rot = glm::rotate(s.M_rot, glm::radians(vel.x), right_);
                s.M_rot = glm::rotate(s.M_rot, glm::radians(vel.y), s.up);
            }
        }

        // Process input
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

        // Declare UI
        if (s.showUI) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                // ImGui::SetNextWindowSize(ImVec2(270, 80));
                ImGui::Begin("Debug Info (Press H to hide/show)", nullptr,
                             ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoScrollbar);
                ImGui::Text("Pitch: %.1f°, Yaw: %.1f°, FoV: %.0f°", 0.0f, 0.0f,
                            s.fov);
                ImGui::Text("Average %.2f ms/frame (%.1f FPS)", 1000.0f / fps,
                            fps);
                // ImGui::Text("Window focused: %s", ImGui::IsWindowFocused());
                ImGui::Text("M_rot: ");
                ImGui::SameLine();
                ImGui::Text(
                    "%s", utils::pretty_matrix(glm::value_ptr(s.M_rot), 4, 4, 2)
                              .c_str());
                ImGui::Text("|right|: %f", glm::length(s.right));
                ImGui::Text("accel: %s", glm::to_string(accel).c_str());
                ImGui::Text("vel: %s", glm::to_string(vel).c_str());
                ImGui::Text("pose: %s", glm::to_string(pose).c_str());

                ImGui::Checkbox("Enable random trajectory",
                                &s.randomTrajectory);

                if (ImGui::Button("Randomize rotation")) {
                    float pitch = glm::linearRand(-50.0f, 50.0f);
                    float yaw = glm::linearRand(-180.0f, 180.0f);
                    glm::mat4 n {1.0f};
                    n = glm::rotate(n, glm::radians(pitch), {1, 0, 0});
                    n = glm::rotate(n, glm::radians(yaw),
                                    glm::vec3(glm::row(n, 1)));
                    s.M_rot = n;
                }
                ImGui::End();
            }

            // Render UI
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window.get());

        if (!window.visible()) {
            /*
            Swap again so both the front and back buffer is guaranteed to
            contain the latest frame, which is then read by glReadPixels
            below. If we omit this, glReadPixels may return an empty frame on
            some platforms (because it read the other buffer which was not
            rendered to yet).
            */
            glfwSwapBuffers(window.get());
            break;
        }
    }

    auto [w, h] = window.frameBufferShape();
    LOG("Frame buffer shape: ", w, " ", h);

    {
        Image frame(w, h, 3);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame.data());
        frame.write("out.jpg");
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
