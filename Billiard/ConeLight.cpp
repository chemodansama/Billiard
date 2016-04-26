#include "StdAfx.h"
#include "ConeLight.h"

#include <cmath>

#include "glm\gtc\matrix_transform.hpp"

namespace billiard {

ConeLight::ConeLight(void) : spotExponent_(40)
{
    setSpotCutoff(45);
    direction_ = glm::vec3(0, 0, -1);
}

void ConeLight::setSpotCutoff(float angleInDegrees) {
    spotCutoff_ = angleInDegrees;
    tanPhi_ = std::tan(angleInDegrees * static_cast<float>(M_PI) / 180);
}

glm::mat4 ConeLight::computeProjViewMat() const {
    auto depthProjMat = glm::perspective(spotCutoff_ * 2, 1.0f, 1.0f, length());
    auto depthView = glm::lookAt(glm::vec3(position_), glm::vec3(0), glm::vec3(0, 1, 0));
    return depthProjMat * depthView;
}

void ConeLight::bind(const glsl::Program &program, const Frustum &frustum) const {
    auto viewSpacePos = frustum.getView() * position_;
    program.setUniformVec3("u_Light0Pos", glm::value_ptr(viewSpacePos));
    program.setUniformFloat("u_Light0SpotExp", spotExponent_);

    auto spotDir = frustum.getNormal() * direction_;
    program.setUniformVec3("u_Light0SpotDir", glm::value_ptr(spotDir));
    
    program.setUniformFloat("u_Light0SpotCosCutoff", static_cast<float>(std::cos(spotCutoff_ * M_PI / 180)));
}

}