#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <memory>
#include <functional>

namespace billiard {

bool isFramebufferOk(GLenum err);

class Renderbuffer {
    static GLuint* getRenderbuffer() {
		auto t = new GLuint;
		glGenRenderbuffers(1, t);
		return t;
	}

	static std::function<void(const GLuint*)> getDeleter() {
		return [](const GLuint *texture){
            glDeleteRenderbuffers(1, texture);
			delete texture;
		};
	}

    std::unique_ptr<GLuint, decltype(getDeleter())> renderbuffer_;

    template <GLenum target>
    static void checkTarget() {
        static_assert(target == GL_RENDERBUFFER 
            || target == GL_RENDERBUFFER_EXT, 
            "wrong target");
    };

public:
    Renderbuffer(void) : renderbuffer_(getRenderbuffer(), getDeleter()) {}
    Renderbuffer(Renderbuffer&& f) : renderbuffer_(std::move(f.renderbuffer_)) {}
    Renderbuffer &operator=(Renderbuffer&& f) {
        renderbuffer_ = std::move(f.renderbuffer_);
        return *this;
    };

    operator GLuint() const { return *renderbuffer_.get(); }
};


class Framebuffer
{
    static GLuint* getFramebuffer() {
		auto t = new GLuint;
		glGenFramebuffers(1, t);
		return t;
	}

	static std::function<void(const GLuint*)> getDeleter() {
		return [](const GLuint *texture){
            glDeleteFramebuffers(1, texture);
			delete texture;
		};
	}

	std::unique_ptr<GLuint, decltype(getDeleter())> framebuffer_;

	template <GLenum target>
	static void checkTarget() {
        static_assert(target == GL_FRAMEBUFFER_EXT 
            || target == GL_FRAMEBUFFER, 
            "wrong target");
	};
public:
    Framebuffer(void) : framebuffer_(getFramebuffer(), getDeleter()) {}
    Framebuffer(Framebuffer&& f) : framebuffer_(std::move(f.framebuffer_)) {}
    Framebuffer &operator=(Framebuffer&& f) {
        framebuffer_ = std::move(f.framebuffer_);
        return *this;
    };

    operator GLuint() const { return *framebuffer_.get(); }

	template <GLenum target> 
	void bind() const {
		checkTarget<target>();
		glBindFramebuffer(target, *this);
	}

    template <GLenum target> 
    static void unbind() {
        checkTarget<target>();
        glBindFramebuffer(target, 0);
    }
};

}