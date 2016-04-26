#pragma once

#include <glm\glm.hpp>

namespace billiard {

class Plane
{
    union {
        struct {
            float x_, y_, z_, w_;
        };
        float v[4];
    };
public:
    Plane(void) : x_(0), y_(0), z_(0), w_(0) {}
    Plane(const glm::vec4 &v) : x_(v.x), y_(v.y), z_(v.z), w_(v.w) {}
    Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2);

    float normalLength() const;
    bool getDirectiveVectors(glm::vec3 &u, glm::vec3 &v) const;

    const float* data() const { return &v[0]; }
    float x() const { return x_; }
    float y() const { return y_; }
    float z() const { return z_; }
    float w() const { return w_; }
};

}