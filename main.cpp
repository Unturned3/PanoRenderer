
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <string>
#include "utils.h"
#include "shader_tools.h"

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
    GLFWwindow *window;  // created window

    glfwSetErrorCallback(glfwErrorCallback_);
    if (glfwInit() == 0) {
        std::cerr << "GLFW failed to initialize!" << std::endl;
        return -1;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // OSX only?
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(640, 480, "GLFW", nullptr, nullptr);

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

    LOG("OpenGL version: ", glGetString(GL_VERSION));

    uint VAO;
    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    // Four corners of a square.
    float pos[] = {
        0.5, 0.5,
        -0.5, 0.5,
        -0.5, -0.5,
        0.5, -0.5,
    };

    uint idx[] = {
        0, 1, 2,    // 1st triangle
        0, 2, 3,    // 2nd triangle
    };

    uint VBO;
    glGenBuffers(1, &VBO);  
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);

    if (GLEW_KHR_debug) {
        glDebugMessageCallback(glErrorCallback_, nullptr);
    } else {
        LOG("glDebugMessageCallback not available.");
    }

    uint shader = create_shader(
        "../shaders/vertex.glsl",
        "../shaders/frag.glsl"
    );
    glUseProgram(shader);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          2*sizeof(float), (const void*)0);
    
    uint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // Don't fill (i.e. draw a wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (glfwWindowShouldClose(window) == 0) {
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
