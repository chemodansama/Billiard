#pragma once

#include <string>
#include <algorithm>
#include <initializer_list>

#include <GL/glew.h>
#include <GL/gl.h>

#include <vector>
#include <utility>
#include <exception>

#include <glm/glm.hpp>

namespace glsl {

namespace detail {
    
template <typename It>
void compileShader(GLuint shader, It sourceBegin, It sourceEnd) {
    auto size = std::distance(sourceBegin, sourceEnd);

    std::vector<GLint> lengths;
    std::transform(sourceBegin, sourceEnd, std::back_inserter(lengths), 
        [](const std::string &s){ return s.size(); });

    std::vector<const char*> csource;
    std::transform(sourceBegin, sourceEnd, std::back_inserter(csource), 
        [](const std::string &s){ return s.c_str(); });

    glShaderSource(shader, size, csource.data(), lengths.data());
    glCompileShader(shader);
    logShaderInfo(shader);
}

}

class Uniform {
private:
    std::string mName;
    int mLocation;
public:
    Uniform(const std::string &name, int location)
        : mName(name)
        , mLocation(location) {
    }

    const std::string &name() const  { return mName; }
    int location() const  { return mLocation; }
};

std::string loadShaderFromFile(const std::string &fileName);

class Shader {
public:
    template <GLuint type>
    static Shader create(const std::vector<std::string> &sources) {
        return create<type>(sources.begin(), sources.end());
    }

    template <GLuint type, typename It>
    static Shader create(It sourceBegin, It sourceEnd) {
        static_assert(type == GL_VERTEX_SHADER 
                || type == GL_FRAGMENT_SHADER
                || type == GL_TESS_CONTROL_SHADER
                || type == GL_TESS_EVALUATION_SHADER, 
            "wrong type");

        auto shader = glCreateShader(type);
        detail::compileShader(shader, sourceBegin, sourceEnd);
        if (!checkCompileStatus(shader)) {
            glDeleteShader(shader);
            throw std::runtime_error("failed to compile shader");
        }
        return shader;
    }
    Shader() : shader(0) {}
    Shader(Shader&) = delete;
    Shader(Shader&& s) : shader(s.shader) { s.shader = 0; }
    Shader &operator=(Shader&) = delete;
    Shader &operator=(Shader&& s) { release(); shader = s.shader; s.shader = 0; }
    ~Shader() { release(); }

    operator GLuint() const { return shader; }
    void release() {
        if (shader) { 
            glDeleteShader(shader); 
            shader = 0;
        }
    }
private:
    Shader(GLuint shader) : shader(shader) {}
    GLuint shader;
};

class Program {
private:
    Shader mVertexShader;
    Shader mTessControlShader;
    Shader mTessEvalShader;
    Shader mFragmentShader;
    GLuint mProgram;

    std::vector<Uniform> mUniforms;
    std::vector<Uniform> mAttributes;
public:
    Program(const std::string &defines, const std::string &vertexSource, const std::string &fragmentSource);
    Program(const std::string &defines, 
            const std::string &vertexSource, const std::string &tessControlSource,
            const std::string &tessEvalSource, const std::string &fragmentSource);
    Program(const Program&); // = delete;
    Program(Program&&); // = delete; // can be implemented though.
    Program& operator=(const Program&); // = delete;
    Program& operator=(Program&&); // = delete; // can be implemented though.
    ~Program();

    static void unbind() { glUseProgram(0); }

    void bind() const;
    
    void setUniformFloat(const std::string &name, float value) const;
    void setUniformFloat(const int location, float value) const;
    
    void setUniformInt(const std::string &name, int value) const;
    void setUniformInt(const int location, int value) const;
    
    void setUniformMat4(const std::string &name, bool transpose, const float *value) const;
    void setUniformMat3(const std::string &name, bool transpose, const float *value) const;

    void setUniformVec4(const std::string &name, const float *value, const int count = 1) const;
    void setUniformVec4(int location, const float *value, const int count = 1) const;
    void setUniformVec3(const std::string &name, const float *value) const;

    bool setAttrPtr(const std::string &name, int numComponents, GLsizei stride, void *ptr, 
        GLenum type = GL_FLOAT, bool normalized = false) const;
    static void setAttrPtr(GLuint index, int numComponents, GLsizei stride, void *ptr, 
        GLenum type = GL_FLOAT, bool normalized = false);

    int getUniformLocation(const std::string &name) const;
    int getAttribLocation(const std::string &name) const;
};

}