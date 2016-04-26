#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <memory>
#include <functional>

namespace billiard {

class Texture
{
    static GLuint* getTexture() {
        auto t = new GLuint;
        glGenTextures(1, t);
        return t;
    }

    static std::function<void(const GLuint*)> getDeleter() {
        return [](const GLuint *texture){
            glDeleteTextures(1, texture);
            delete texture;
        };
    }

    std::unique_ptr<GLuint, decltype(getDeleter())> texture;

    template <GLenum target>
    static void checkTarget() {
        static_assert(target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE, 
            "wrong target");
    };
public:
    Texture(void) : texture(getTexture(), getDeleter()) {}
    Texture(Texture& t); // = delete;
    Texture(Texture&& t) : texture(std::move(t.texture)) {}
    Texture &operator=(Texture&& t) {
        texture = std::move(t.texture);
        return *this;
    }

    operator GLuint() const { return *texture.get(); }

    template <GLenum target> 
    void bind() const {
        checkTarget<target>();
        glBindTexture(target, *this);
    }
};

}