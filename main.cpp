
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

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = 0.0f;
float pitch = 0.0f;

int main()
{

    Window window(600, 600, "OpenGL Test");

    if (glewInit() != GLEW_OK)
        throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug) {
        glDebugMessageCallback(glErrorCallback_, nullptr);
    } else {
        LOG("glDebugMessageCallback not available.");
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* img = stbi_load(utils::path("images/container.jpg").c_str(),
        &width, &height, &channels, 0);
    if (!img) {
        throw std::runtime_error("stbi_load() failed!");
    }
    assert(channels == 3);

    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
        GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img);

    // Column 1,2,3: x,y coords of the points of a rectangle
    // Column 4,5,6: RGB colors of the corresponding points
    // Column 7,8: texture coordinates
    constexpr int stride = 5;
    VertexBuffer vb(cube_vertices, sizeof(cube_vertices));

    Shader shader(
        utils::path("shaders/vertex.glsl"), utils::path("shaders/frag.glsl"));
    shader.use();

    uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float),
        (void*)(3 * sizeof(float)));

    vb.bind();

    // clang-format off
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f), 
        glm::vec3( 2.0f,  5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f),  
        glm::vec3(-3.8f, -2.0f, -12.3f),  
        glm::vec3( 2.4f, -0.4f, -3.5f),  
        glm::vec3(-1.7f,  3.0f, -7.5f),  
        glm::vec3( 1.3f, -2.0f, -2.5f),  
        glm::vec3( 1.5f,  2.0f, -2.5f), 
        glm::vec3( 1.5f,  0.2f, -1.5f), 
        glm::vec3(-1.3f,  1.0f, -1.5f)  
    };
    // clang-format on

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    vb.unbind();

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glm::mat4 M_proj = glm::perspective(
        glm::radians(65.0f), (float)width / float(height), 0.5f, 100.0f);
    shader.setMat4("proj", M_proj);

    while (glfwWindowShouldClose(window.get()) == 0) {

        processInput(window.get());

        glm::mat4 M_view
            = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("view", M_view);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);

        for (int i = 0; i < 10; i++) {

            float angle = 20.0f * static_cast<float>(i);

            glm::mat4 M_model = glm::mat4(1.0f);
            M_model = glm::translate(M_model, cubePositions[i]);
            M_model = glm::rotate(
                M_model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

            shader.setMat4("model", M_model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

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
    const float cameraSpeed = 0.05f; // adjust accordingly
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos
            -= glm::normalize(glm::cross(cameraRight, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos
            += glm::normalize(glm::cross(cameraRight, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= cameraRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += cameraRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;

    glm::vec3 direction;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        pitch += 1;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        pitch -= 1;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        yaw += 1;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        yaw -= 1;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    direction.x = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = direction;
}
