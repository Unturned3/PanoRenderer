
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utils.hpp"

class IndexBuffer {
public:
    IndexBuffer(const uint* data, uint count, GLenum usage = GL_STATIC_DRAW)
        : count_(count)
    {
        glGenBuffers(1, &id_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint), data,
                     usage);
    }

    ~IndexBuffer() { glDeleteBuffers(1, &id_); }

    IndexBuffer(const IndexBuffer& o) = delete;
    IndexBuffer& operator=(const IndexBuffer& o) = delete;

    void bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_); }

    void unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

    const uint& count() const { return count_; }

private:
    uint id_;
    uint count_;
};
