
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
#include "VertexBuffer.hpp"
#include "Window.hpp"
#include "Sphere.hpp"
#include "cube.h"
#include "utils.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

static void glErrorCallback_(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length, const GLchar* msg, const void* userParam)
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

int main(int argc, char **argv)
{
    const int w_w = 640, w_h = 480;
    Window window(w_w, w_h, "OpenGL Test");

    if (glewInit() != GLEW_OK)
        throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug)
        glDebugMessageCallback(glErrorCallback_, nullptr);
    else
        LOG("glDebugMessageCallback not available.");

    std::string filePath = argc < 2 ? "images/p1.jpg" : argv[1];
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* img = stbi_load(
        utils::path(filePath).c_str(),
        &width, &height, &channels, 0);

    if (!img) throw std::runtime_error("stbi_load() failed!");
    assert(channels == 3);

    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
        GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img);

    // Rectangle (two triangles) covering the screen.
    constexpr int stride = 3;
    std::vector<float> vertices {
        -1, -1,  0,
         1,  1,  0,
        -1,  1,  0,
        -1, -1,  0,
         1, -1,  0,
         1,  1,  0,
    };

    VertexBuffer vb(vertices.data(),
        static_cast<uint>(vertices.size() * sizeof(float)));

    Shader shader(
        utils::path("shaders/vertex.glsl"), utils::path("shaders/frag.glsl"));
    shader.use();

    uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);

    vb.bind();

    while (glfwWindowShouldClose(window.get()) == 0) {

        processInput(window.get());

        glm::mat4 M_proj = glm::perspective(
            glm::radians(fov), (float)w_w / (float)w_h, 0.01f, 100.0f);
        M_proj = glm::inverse(M_proj);
        shader.setMat4("inv_proj", M_proj);

        glm::mat4 M_view
            = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("view", M_view);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLES, 0,
            static_cast<int>(vertices.size() / stride));

        glfwSwapBuffers(window.get());
        glfwPollEvents();
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

    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        fov -= 1;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        fov += 1;
    
    fov = std::min(120.0f, fov);
    fov = std::max(10.0f, fov);
    float cam_rot_speed = 1.2f - (120 - fov) / 120.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pitch += cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pitch -= cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        yaw += cam_rot_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        yaw -= cam_rot_speed;

    pitch = std::min(89.0f, pitch);
    pitch = std::max(-89.0f, pitch);

    float rp = glm::radians(pitch);
    float ry = glm::radians(yaw);

    glm::vec3 direction {
        -sin(ry) * cos(rp),
         sin(rp),
        -cos(ry) * cos(rp),
    };
    cameraFront = direction;
}
