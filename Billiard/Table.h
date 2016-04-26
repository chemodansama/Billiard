#pragma once

#include <string>
#include <GL\glew.h>
#include <GL\GL.h>

#include "GlslProgram.h"
#include "VertexBuffer.h"
#include "VertexArray.h"
#include "Frustum.h"
#include "ConeLight.h"
#include "Texture.h"

namespace billiard {

class Table
{
    const VertexArray vao_;
    const VertexBuffer vbo_;
    const glsl::Program program_;
    const glsl::Program depth_;
    const Texture texture_;
public:
    Table(const std::string &exePath);

    void render(const Frustum &frustum, const ConeLight &light, 
        const Texture &shadowMap);

    void renderDepth(const Frustum &frustum);
};

}

