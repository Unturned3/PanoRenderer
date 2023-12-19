
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <string>

#include "utils.h"
#include "shader_tools.h"

int main() {
    GLFWwindow *window;  // created window

    if (glfwInit() == 0) {
        std::cerr << "GLFW failed to initiate." << std::endl;
        return -1;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(640, 480, "GLFW", nullptr, nullptr);

    // check if window was created successfully
    if (window == nullptr) {
        std::cerr << "GLFW failed to create window." << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLFW failed to create window." << std::endl;
        glfwTerminate();
        return -1;
    }

    LOG("OpenGL version: ", glGetString(GL_VERSION));

    uint VAO;
    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    // Vertex data and buffer
    float pos[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.0f,  0.5f,
    };

    uint VBO;
    glGenBuffers(1, &VBO);  
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);

    uint shader = createShader(
        "../shaders/vertex.glsl",
        "../shaders/frag.glsl"
    );
    glUseProgram(shader);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          2*sizeof(float), (const void*)0);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    while (glfwWindowShouldClose(window) == 0) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}