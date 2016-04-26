#include "StdAfx.h"
#include "GlslProgram.h"

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include "utils.h"

#include <glog/logging.h>
using namespace google;

namespace glsl {

void logShaderInfo(GLuint shader) {
    GLint size;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
    if (!size) {
        return;
    }

    std::vector<char> infoLog(size);
    GLsizei infoLen;
    glGetShaderInfoLog(shader, size, &infoLen, infoLog.data());
    
    LOG(ERROR) << std::string(infoLog.begin(), infoLog.end());
}

template <typename T, typename V>
std::vector<std::string> createSources(T&& defines, V&& source) {
    std::vector<std::string> result;
    result.push_back("#version 410 core\n");
    result.push_back(std::forward<T>(defines));
    result.push_back(std::forward<V>(source));
    return result;
}
   
bool checkCompileStatus(GLuint shader) {
    GLint compileStatus;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        // some error
        LOG(ERROR) <<  "Error compiling shader";
        return false;
    }
    return true;
}

std::vector<Uniform> loadAttributeLocations(GLuint program) {
    std::vector<Uniform> result;
    int attributesCount;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attributesCount);

    GLint maxLen = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen);

    std::vector<GLchar> name(maxLen);
    for (int i = 0; i < attributesCount; i++) {
        GLsizei len;
        GLint size;
        GLenum type;

        glGetActiveAttrib(program, i, maxLen,  &len, &size, &type, name.data());
        Uniform p(std::string(name.begin(), name.begin() + len), 
                    glGetAttribLocation(program, name.data()));
        result.push_back(std::move(p));

        LOG(INFO) << "Attribute(" << p.name() << ") located at " << p.location();
    }
    return result;
}

std::vector<Uniform> loadUniformLocations(GLuint program) {
    std::vector<Uniform> result;

    GLint uniformsCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformsCount);

    GLint maxLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    std::vector<GLchar> name(maxLength);
    for (int i = 0; i < uniformsCount; i++) {
        GLsizei len;
        GLint size;
        GLenum type;

        glGetActiveUniform(program, i, maxLength,  &len, &size, &type, name.data());
        Uniform p(std::string(name.begin(), name.begin() + len), 
                    glGetUniformLocation(program, name.data()));
        result.push_back(p);

        LOG(INFO) << "Uniform(" << p.name() << ") located at " << p.location();
    }
    return result;
}

GLuint compileProgram(GLuint vs, GLuint tcs, GLuint tes, GLuint fs) {
    auto program = glCreateProgram();
    glAttachShader(program, vs);
    if (tcs) {
        glAttachShader(program, tcs);
    }

    if (tes) {
        glAttachShader(program, tes);
    }

    glAttachShader(program, fs);
    glLinkProgram (program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        throw std::runtime_error("Error linking program");
    }

    return program;
}

Program::Program(const std::string &defines, const std::string &vertexSource, 
        const std::string &fragmentSource) 
        : mVertexShader(Shader::create<GL_VERTEX_SHADER>(createSources(defines, vertexSource)))
        , mFragmentShader(Shader::create<GL_FRAGMENT_SHADER>(createSources(defines, fragmentSource)))
        , mProgram(compileProgram(mVertexShader, 0, 0, mFragmentShader))
        , mAttributes(loadAttributeLocations(mProgram))
        , mUniforms(loadUniformLocations(mProgram)) {
}

Program::Program(const std::string &defines, const std::string &vertexSource, const std::string &tessControlSource,
        const std::string &tessEvalSource, const std::string &fragmentSource)
        : mVertexShader(Shader::create<GL_VERTEX_SHADER>(createSources(defines, vertexSource)))
        , mTessControlShader(Shader::create<GL_TESS_CONTROL_SHADER>(createSources(defines, tessControlSource)))
        , mTessEvalShader(Shader::create<GL_TESS_EVALUATION_SHADER>(createSources(defines, tessEvalSource)))
        , mFragmentShader(Shader::create<GL_FRAGMENT_SHADER>(createSources(defines, fragmentSource)))
        , mProgram(compileProgram(mVertexShader, mTessControlShader, mTessEvalShader, mFragmentShader))
        , mAttributes(loadAttributeLocations(mProgram))
        , mUniforms(loadUniformLocations(mProgram)) {
}

Program::~Program() {
    glDeleteProgram(mProgram);
}

void Program::setAttrPtr(GLuint index, int numComponents, GLsizei stride, void *ptr, 
        GLenum type, bool normalized) {
    glVertexAttribPointer(index,                // index
                          numComponents,        // number of values per vertex
                          type,                 // type (GL_FLOAT)
                          normalized ? GL_TRUE : GL_FALSE,
                          stride,               // stride (offset to next vertex data)
                          (const GLvoid*) ptr);
    glEnableVertexAttribArray(index);
}

bool Program::setAttrPtr(const std::string &name, int numComponents, 
        GLsizei stride, void *ptr, GLenum type, bool normalized) const {
    if (!mProgram) {
        LOG(INFO) << "program is invalid";
        return false;
    }

    int loc = getAttribLocation(name);
    if (loc < 0) {
        LOG(INFO) << name << " is not active attribute";
        return false;
    }

    setAttrPtr(static_cast<GLuint>(loc), numComponents, stride, ptr, type, normalized);
    return true;
}

void Program::setUniformFloat(const int location, float value) const {
    glUniform1f(location, value);
}

void Program::setUniformFloat(const std::string &name, float value) const {
    setUniformFloat(getUniformLocation(name), value);
}

void Program::setUniformInt(const std::string &name, int value) const {
    setUniformInt(getUniformLocation(name), value);
}

void Program::setUniformInt(const int location, int value) const {
    if (location < 0) {
        LOG(INFO) << location << " is not valid uniform location";
        return;
    }
    glUniform1i(location, value);
}

void Program::setUniformMat3(const std::string &name, bool transpose, const float *value) const {
    int loc = getUniformLocation(name);
    if (loc < 0) {
        LOG(WARNING) << "Unknown uniform : " << name;
        return;
    }
    glUniformMatrix3fv(loc, 1, transpose, value);
}

void Program::setUniformMat4(const std::string &name, bool transpose, const float *value) const {
    int loc = getUniformLocation(name);
    if (loc < 0) {
        LOG(WARNING) << "Unknown uniform : " << name;
        return;
    }
    glUniformMatrix4fv(loc, 1, transpose, value);
}

void Program::setUniformVec4(int location, const float *value, const int count) const {
    if (location < 0) {
        LOG(WARNING) << "Unknown uniform : " << location;
        return;
    }
    glUniform4fv(location, count, value);
}

void Program::setUniformVec4(const std::string &name, const float *value, const int count) const {
    auto location = getUniformLocation(name);
    if (location < 0) {
        LOG(WARNING) << "Unknown uniform : " << name;
        return;
    }
    setUniformVec4(location, value, count);
}

void Program::setUniformVec3(const std::string &name, const float *value) const {
    auto loc = getUniformLocation(name);
    if (loc < 0) {
        LOG(WARNING) << "Unknown uniform : " << name;
        return;
    }
    glUniform3fv(loc, 1, value);
}

void Program::bind() const {
    if (!mProgram) {
        return;
    }
    glUseProgram(mProgram);
}

int Program::getAttribLocation(const std::string &name) const {
    for (auto it = mAttributes.begin(); it != mAttributes.end(); ++it) {
        if (it->name().compare(name) == 0) {
            return it->location();
        }
    }
    return -1;
}

int Program::getUniformLocation(const std::string &name) const {
    for (auto it = mUniforms.begin(); it != mUniforms.end(); ++it) {
        if (it->name().compare(name) == 0) {
            return it->location();
        }
    }
    return -1;
}

std::string loadShaderFromFile(const std::string &fileName) {
    auto data = utils::loadAsset(fileName);
    return std::string(data.begin(), data.end());
}

}