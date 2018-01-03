/*
 * VertexBuffer.h
 *
 *  Created on: 10.08.2013
 *      Author: NullPointer
 */
#pragma once

#include <GL\glew.h>
#include <GL\GL.h>

#include <memory>
#include <functional>
#include <utility>

namespace billiard {

class VertexBuffer {
private:
    static GLuint* getVbo() {
        auto t = new GLuint;
        glGenBuffers(1, t);
        return t;
    }

    static std::function<void(const GLuint*)> getDeleter() {
        return [](const GLuint *buffer){
            glDeleteBuffers(1, buffer);
            delete buffer;
        };
    }

    std::unique_ptr<GLuint, decltype(getDeleter())> vbo;

    template <GLenum target>
    static void checkTarget() {
        static_assert(target == GL_ARRAY_BUFFER 
                || target == GL_COPY_READ_BUFFER 
                || target == GL_COPY_WRITE_BUFFER 
                || target == GL_ELEMENT_ARRAY_BUFFER 
                || target == GL_PIXEL_PACK_BUFFER
                || target == GL_PIXEL_UNPACK_BUFFER 
                || target == GL_TEXTURE_BUFFER 
                || target == GL_TRANSFORM_FEEDBACK_BUFFER 
                || target == GL_UNIFORM_BUFFER,
            "wrong target");
    };
public:
    VertexBuffer() : vbo(getVbo(), getDeleter()) {};
    VertexBuffer(VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& v) : vbo(std::move(v.vbo)) {}

    VertexBuffer &operator=(VertexBuffer&) = delete;
    VertexBuffer &operator=(VertexBuffer&& v) { vbo = std::move(v.vbo); return *this; }

    template <GLenum target, typename V>
    static VertexBuffer create(const V *data, size_t length) {
        VertexBuffer v;
        checkTarget<target>();
        v.bind<target>();
        v.bufferData<target>(data, length);
        VertexBuffer::unbind<target>();
        return v;
    }

    template <GLenum target, typename V>
    void bufferData(const V *data, size_t length) const {
        checkTarget<target>();
        glBufferData(target, length * sizeof(V), data, GL_STATIC_DRAW);
    }

    template <GLenum target>
    void bind() const {
        checkTarget<target>();
        glBindBuffer(target, *vbo);
    }

    template <GLenum target>
    static void unbind() {
        checkTarget<target>();
        glBindBuffer(target, 0);
    }
};

} /* namespace zengine */
