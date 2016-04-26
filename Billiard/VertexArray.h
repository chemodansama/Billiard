#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

namespace billiard {

/**
* Ctor-dtor wrapper for vertex array object.
*/ 
class VertexArray
{
public:
    VertexArray(void) { 
        glGenVertexArrays(1, &vao_); 
    }
    VertexArray(VertexArray&) = delete;
    VertexArray(VertexArray&& v) : vao_(v.vao_) { v.vao_ = 0; }
    VertexArray &operator=(VertexArray&) = delete;
    VertexArray &operator=(VertexArray&& v) { release(); vao_ = v.vao_; v.vao_ = 0; }

    ~VertexArray(void) { release(); }

    operator GLuint() const { return vao_; }

private:
    GLuint vao_;

    void release() {
        if (vao_) {
            glDeleteVertexArrays(1, &vao_);
        }
    }
};

}
