#pragma once

#include <vector>
#include <utility>

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "GlslProgram.h"
#include "Frustum.h"
#include "Texture.h"
#include "ConeLight.h"

#define BALL_DIAMETER 0.68f

namespace billiard {

class Ball
{
    class Programs {
        Programs(const Programs&) = delete;
        Programs& operator=(const Programs&) = delete;
    public:
        const glsl::Program shadow_;
        const glsl::Program normal_;
        const glsl::Program depth_;

        Programs(const std::string &vertexSource, const std::string &tessControlSource,
                const std::string &tessEvalSource, const std::string &fragmentSource) 
                : shadow_("#define SHADOW_PASS\n", vertexSource, tessControlSource, tessEvalSource, fragmentSource)
                , depth_("#define DEPTH_PASS\n", vertexSource, tessControlSource, tessEvalSource, fragmentSource)
                , normal_("", vertexSource, tessControlSource, tessEvalSource, fragmentSource) {
        }
    };

    const std::pair<std::vector<GLfloat>, std::vector<GLushort>> data_;

    const VertexArray vao_;
    const VertexBuffer vbo_;
    const VertexBuffer indices_;
    const Programs programs_;
    const Texture albedo_;
    bool lineFill_;

    const glm::mat4 modelMat_;
    glm::mat4 depthModelViewProj_;
public:
    Ball(const std::string &exePath);

    void render() const;
    void renderShadow() const;
    void renderDepth() const;

    void setLineFill(bool value);
    void update(const Frustum &frustum, const ConeLight &light);

    const glm::mat4 getDepthModelViewProj() const { return depthModelViewProj_; }
};

}