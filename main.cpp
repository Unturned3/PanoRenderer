
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "IndexBuffer.hpp"
#include "Shader.hpp"
#include "Sphere.hpp"
#include "VertexBuffer.hpp"
#include "Window.hpp"
#include "cube.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "utils.hpp"

static void glErrorCallback_(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length, const GLchar* msg,
                             const void* userParam)
{
    LOG("OpenGL error:");
    LOG(msg);
}

void processInput(GLFWwindow* window);

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = 0.0f;
float pitch = 0.0f;
float fov = 75.0f;
float max_fov = 120.0f;

int main(int argc, char** argv)
{
    const int w_w = 640, w_h = 480;
    Window window(w_w, w_h, "OpenGL Test", argc <= 3);
    glfwSwapInterval(1);

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

    std::string filePath = argc < 2 ? "images/p1.jpg" : argv[1];
    int img_w, img_h, img_channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* img = stbi_load(utils::path(filePath).c_str(), &img_w, &img_h,
                             &img_channels, 0);

    if (!img) throw std::runtime_error("stbi_load() failed!");
    assert(img_channels == 3);

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_w, img_h, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img);

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
        double u = (double)img_w / 2 / (double)img_h;
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
    glClearColor(0, 0, 0, 0);

    while (glfwWindowShouldClose(window.get()) == 0) {
        // Process input
        glfwPollEvents();
        processInput(window.get());

        // Clear frame
        glClear(GL_COLOR_BUFFER_BIT);

        // Recalculate FoV, LoD, perspective, & view.
        float fov_thresh = 75.0f;
        if (fov <= fov_thresh)
            shader.setFloat("lod", 0.0f);
        else
            shader.setFloat("lod",
                            (fov - fov_thresh) / (max_fov - fov_thresh) + 1.0f);

        glm::mat4 M_proj = glm::perspective(
            glm::radians(fov), (float)w_w / (float)w_h, 0.1f, 2.0f);
        shader.setMat4("proj", M_proj);

        glm::mat4 M_view =
            glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("view", M_view);

        // Draw projected panorama
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<int>(vertices.size() / stride));

        // Declare UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::SetNextWindowSize(ImVec2(265, 75));
            ImGui::Begin("Debug Info", nullptr,
                         ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("Pitch: %.1f°, Yaw: %.1f°, FoV: %.0f°", pitch, yaw,
                        fov);
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                        1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Window focused: %s",
                        ImGui::IsWindowFocused() ? "yes" : "no");
            ImGui::End();
        }

        // Render UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window.get());

        ImGui::SetWindowFocus(nullptr);
        if (!window.visible()) {
            glfwSwapBuffers(window.get());
            break;
        }
    }

    int w, h;
    glfwGetFramebufferSize(window.get(), &w, &h);
    LOG("WH: ", w, " ", h);

    auto imgOut = std::make_unique<uint8_t[]>(static_cast<size_t>(w * h * 3));
    assert(imgOut.get());

    // glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, imgOut.get());
    stbi_flip_vertically_on_write(true);
    stbi_write_jpg("out.jpg", w, h, 3, imgOut.get(), 90);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) fov -= 1;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) fov += 1;

    fov = std::min(max_fov, fov);
    fov = std::max(10.0f, fov);
    float cam_rot_speed = 1.2f - (max_fov - fov) / max_fov;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pitch += cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pitch -= cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) yaw += cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) yaw -= cam_rot_speed;

    pitch = std::min(89.0f, pitch);
    pitch = std::max(-89.0f, pitch);

    float rp = glm::radians(pitch);
    float ry = glm::radians(yaw);

    glm::vec3 direction{
        -sin(ry) * cos(rp),
        sin(rp),
        -cos(ry) * cos(rp),
    };
    cameraFront = direction;
}
