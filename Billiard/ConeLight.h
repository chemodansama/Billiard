#pragma once

#include <glm\glm.hpp>

#include "GlslProgram.h"
#include "Frustum.h"

namespace billiard {

class ConeLight
{
    float tanPhi_;
    float spotCutoff_;
    float spotExponent_;
    glm::vec3 direction_;
    glm::vec4 position_;
    float length_;
public:
    ConeLight(void);

    void setSpotCutoff(float angleInDegrees);
    void setDirection(const glm::vec3 &direction) { direction_ = direction; }
    void setPosition(const glm::vec3 &position) { position_ = glm::vec4(position, 1); }
    void setSpotExponent(float exponent) { spotExponent_ = exponent; }
    void setLength(float length) { length_ = length; }

    glm::mat4 computeProjViewMat() const;
    float getSpotCutoff() const { return spotCutoff_; }
    float getTanPhi() const { return tanPhi_; }

    glm::vec3 pos() const { return glm::vec3(position_); }
    const glm::vec3 &dir() const { return direction_; }
    float length() const { return length_; }

    void bind(const glsl::Program &program, const Frustum &frustum) const;
};

}