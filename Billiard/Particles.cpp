#include "StdAfx.h"
#include "Particles.h"

#include <cmath>
#include <utility>
#include <vector>
#include <random>

namespace billiard {

namespace {
    const int PARTICLES_AMOUNT = 1024;

    VertexBuffer createParticles(float length, float radius) {
        std::vector<glm::vec4> v;

        std::random_device rd;
        std::default_random_engine rng(rd()); 
        std::uniform_real_distribution<float> xyDistrib(-radius, radius);
        std::uniform_real_distribution<float> zDistrib(0, length);
        std::uniform_real_distribution<float> timeDistrib(0, static_cast<float>(2 * M_PI));

        for (auto i = 0; i < PARTICLES_AMOUNT; i++) {
            v.push_back(glm::vec4(xyDistrib(rng), xyDistrib(rng), zDistrib(rng), timeDistrib(rng)));
        }

        return std::move(VertexBuffer::create<GL_ARRAY_BUFFER>(v.data(), v.size()));
    }
}

Particles::Particles(const std::string &exePath, float length, float radius) 
        : program_("", 
                   glsl::loadShaderFromFile(exePath + "../assets/shaders/particles.vert"), 
                   glsl::loadShaderFromFile(exePath + "../assets/shaders/particles.frag"))
        , vbo_(createParticles(length, radius)) {
    glBindVertexArray(vao_);
    vbo_.bind<GL_ARRAY_BUFFER>();
    glsl::Program::setAttrPtr(0, 4, 0, nullptr);
    glBindVertexArray(0);
    VertexBuffer::unbind<GL_ARRAY_BUFFER>();
}

void Particles::render(const Frustum &frustum) const {
    glBindVertexArray(vao_);
    program_.bind();
    program_.setUniformMat4("u_ProjectionMat", false, frustum.getProjPtr());
    program_.setUniformMat4("u_ViewMat", false, frustum.getViewPtr());

    glDrawArrays(GL_POINTS, 0, PARTICLES_AMOUNT);

    glsl::Program::unbind();
    glBindVertexArray(0);
}

}