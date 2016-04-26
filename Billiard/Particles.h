#pragma once

#include <sstream>

#include <GL\glew.h>
#include <GL\GL.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "GlslProgram.h"
#include "Frustum.h"

namespace billiard {

class Particles {
    const VertexArray vao_;
    const VertexBuffer vbo_;
    const glsl::Program program_;
public:
    Particles(const std::string &exePath, float length, float radius);

    template <int N>
    void setClipPlanes(glm::vec4 (&lightFrustum)[N]) const {
        program_.bind();
        int loc = program_.getUniformLocation("u_ClipPlanes[0]");
        for (int i = 0; i < 6; i++) {
            program_.setUniformVec4(loc + i, glm::value_ptr(lightFrustum[i]));
        }
        glsl::Program::unbind();
    }

    void render(const Frustum &frustum) const;
};

}
