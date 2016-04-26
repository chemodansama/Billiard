#include "StdAfx.h"
#include "Plane.h"

#define LEAST_UNIT_COMPONENT (1 / sqrt(3.0f))

namespace billiard {

Plane::Plane(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2) {
    glm::vec3 u = p1 - p0;
    glm::vec3 v = p2 - p0;
    glm::vec3 n = glm::cross(u, v);

    float d = -glm::dot(n, p0);

    float normalLength = glm::length(n);
    n /= normalLength;
    d /= normalLength;

    x_ = n.x;
    y_ = n.y;
    z_ = n.z;
    w_ = d;
}

float Plane::normalLength() const
{
    return sqrt(x_ * x_ + y_ * y_ + z_ * z_);
}

bool Plane::getDirectiveVectors(glm::vec3 &u, glm::vec3 &v) const
{
    if (normalLength() < 1.0E-06) {
        return false;
    }

    auto pn = glm::normalize(glm::vec3(x_, y_, z_));
    if (abs(pn.x) > LEAST_UNIT_COMPONENT / 2) { 
        u.x = -pn.y / pn.x;
        u.y = 1;
        u.z = 0;

        v.x = -pn.z / pn.x;
        v.y = 0;
        v.z = 1;
    } else if (abs(pn.y) > LEAST_UNIT_COMPONENT / 2) {
        u.x = 1;
        u.y = -pn.x / pn.y;
        u.z = 0;

        v.x = 0;
        v.y = -pn.z / pn.y;
        v.z = 1;
    } else {
        u.x = 1;
        u.y = 0;
        u.z = -pn.x / pn.z;

        v.x = 0;
        v.y = 1;
        v.z = -pn.y / pn.z;
    }

    return true;
}

}