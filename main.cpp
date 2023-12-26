
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdint>
#include <cassert>
#include <memory>

#include "utils.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

void glErrorCallback_(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, const GLchar *msg, const void *userParam) {
    LOG("OpenGL error:");
    LOG(msg);
}

void glfwErrorCallback_(int err, const char *msg) {
    LOG("GLFW error code: ", err);
    LOG(msg);
}

int main() {

    /*
    if (!GLEW_NV_vdpau_interop)
        LOG("NV_vdpau_interop unavailable!");
    if (!GLEW_NV_vdpau_interop2)
        LOG("NV_vdpau_interop2 unavailable!");
    */

    GLFWwindow *window;  // created window

    glfwSetErrorCallback(glfwErrorCallback_);
    if (glfwInit() == 0) {
        std::cerr << "GLFW failed to initialize!" << std::endl;
        return -1;
    }

    glfwDefaultWindowHints();
    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // OSX only?
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(1024, 512, "GLFW", nullptr, nullptr);

    // Check if window was created successfully
    if (window == nullptr) {
        std::cerr << "GLFW failed to create window!" << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (GLEW_KHR_debug) {
        glDebugMessageCallback(glErrorCallback_, nullptr);
    } else {
        LOG("glDebugMessageCallback not available.");
    }

    {
        LOG("OpenGL version: ", glGetString(GL_VERSION));

        int maxVertAtrbs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertAtrbs);
        LOG("Max number of vertex attributes: ", maxVertAtrbs);

        int maxTexSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
        LOG("Max texture size: ", maxTexSize);

        int max3DTexSize;
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTexSize);
        LOG("Max 3D texture size: ", max3DTexSize);

        int maxRectTexSize;
        glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &maxRectTexSize);
        LOG("Max rectangular texture size: ", maxRectTexSize);
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *img = stbi_load(utils::path("images/pano-1024.jpg").c_str(),
                             &width, &height, &channels, 0);
    if (!img) {
        throw std::runtime_error("stbi_load() failed!");
    }
    assert(channels == 3);

    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img);

    // Column 1,2,3: x,y coords of the points of a rectangle
    // Column 4,5,6: RGB colors of the corresponding points
    // Column 7,8: texture coordinates
    constexpr int stride = 8;
    float vertices[stride * 4] = {
        // top right
         0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.5f, 1.0f, 1.0f,
        // top left
        -0.5f,  0.5f,  0.0f,  1.0f,  0.6f,  0.0f, 0.0f, 1.0f,
        // bottom left
        -0.5f, -0.5f,  0.0f,  0.0f,  0.9f,  0.5f, 0.0f, 0.0f,
        // bottom right
         0.5f, -0.5f,  0.0f,  0.5f,  0.0f,  1.0f, 1.0f, 0.0f,
    };
    VertexBuffer vb(vertices, sizeof(vertices));

    uint indices[] = {
        0, 1, 2,    // 1st triangle
        0, 2, 3,    // 2nd triangle
    };
    IndexBuffer ib(indices, 6);

    Shader shader(utils::path("shaders/vertex.glsl"),
                  utils::path("shaders/frag.glsl"));
    shader.use();

    uint VAO;
    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          stride*sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          stride*sizeof(float), (void*)(3*sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          stride*sizeof(float), (void*)(6*sizeof(float)));

    vb.bind();
    ib.bind();

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    vb.unbind();
    ib.unbind();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glm::mat4 M_model = glm::rotate(glm::mat4(1.0f), glm::radians(-55.0f),
                                    glm::vec3(1.0f, 0.0f, 0.0f)); 
    glm::mat4 M_view = glm::translate(glm::mat4(1.0f),
                                      glm::vec3(0.0f, 0.0f, -2.0f)); 
    glm::mat4 M_proj = glm::perspective(glm::radians(45.0f),
                                        (float)width/float(height),
                                        0.5f, 100.0f);

    shader.setMat4("transform", M_proj * M_view * M_model);

    while (glfwWindowShouldClose(window) == 0) {

        float t = static_cast<float>(glfwGetTime());
        float c = std::sin(t) / 2.0f + 0.5f;
        shader.setFloat("tColor", c);

        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    LOG("WH: ", w, " ", h);

    auto imgOut = std::make_unique<uint8_t[]>(static_cast<size_t>(w*h*3));
    assert(imgOut.get());

    //glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, imgOut.get());
    stbi_flip_vertically_on_write(true);
    stbi_write_jpg("out.jpg", w, h, 3, imgOut.get(), 90);

    glfwTerminate();
    return 0;
}
