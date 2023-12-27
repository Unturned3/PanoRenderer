
#pragma once

#include "utils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class VertexBuffer {
public:
    VertexBuffer(const void* data, uint size, GLenum usage = GL_STATIC_DRAW)
    {
        glGenBuffers(1, &id_);
        glBindBuffer(GL_ARRAY_BUFFER, id_);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    }

    ~VertexBuffer() { glDeleteBuffers(1, &id_); }

    VertexBuffer(const VertexBuffer& o) = delete;
    VertexBuffer& operator=(const VertexBuffer& o) = delete;

    void bind() const { glBindBuffer(GL_ARRAY_BUFFER, id_); }

    void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

private:
    uint id_;
};
