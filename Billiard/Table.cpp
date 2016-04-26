#include "StdAfx.h"

#include "Table.h"

#include "utils.h"

#include <glm\gtc\type_ptr.hpp>

#include "glog\logging.h"

#define TABLE_WIDTH 10.0f
#define TABLE_HEIGHT 10.0f

namespace {
    const float vertices[] =  {
        -TABLE_WIDTH / 2, -TABLE_HEIGHT / 2, 0, 0, 0, 1, 0,  0,
         TABLE_WIDTH / 2, -TABLE_HEIGHT / 2, 0, 0, 0, 1, 10, 0,
         TABLE_WIDTH / 2,  TABLE_HEIGHT / 2, 0, 0, 0, 1, 10, 10,
        -TABLE_WIDTH / 2,  TABLE_HEIGHT / 2, 0, 0, 0, 1, 0,  10
    };
}

namespace billiard {

Table::Table(const std::string &exePath) 
        : program_("", 
                   glsl::loadShaderFromFile(exePath + "../assets/shaders/table.vert"), 
                   glsl::loadShaderFromFile(exePath + "../assets/shaders/table.frag"))
        , depth_("#define DEPTH_PASS\n",
                 glsl::loadShaderFromFile(exePath + "../assets/shaders/table.vert"), 
                 glsl::loadShaderFromFile(exePath + "../assets/shaders/table.frag"))
        , vbo_(VertexBuffer::create<GL_ARRAY_BUFFER>(vertices, utils::length(vertices)))
{
    glBindVertexArray(vao_);
    vbo_.bind<GL_ARRAY_BUFFER>();
    auto compSize = sizeof(vertices[0]);
    glsl::Program::setAttrPtr(0, 3, 8 * compSize, nullptr);
    glsl::Program::setAttrPtr(1, 3, 8 * compSize, (void*)(3 * compSize));
    glsl::Program::setAttrPtr(2, 2, 8 * compSize, (void*)(6 * compSize));
    glBindVertexArray(0);
    VertexBuffer::unbind<GL_ARRAY_BUFFER>();

    texture_.bind<GL_TEXTURE_2D>();
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    unsigned int w, h, bpp;
    auto textureFile = exePath + "../assets/textures/pool.png";
    auto data = utils::loadPng(textureFile.c_str(), &w, &h, &bpp);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Table::renderDepth(const Frustum &frustum) {
    const auto view = frustum.getView();

    depth_.bind();
    depth_.setUniformMat4("u_ModelviewProjectionMat", false, frustum.getViewProjPtr());
    depth_.setUniformMat4("u_ModelviewMat", false, glm::value_ptr(view)); // view is equal to modelView.
    depth_.setUniformFloat("u_NearPlane", frustum.getNear());
    depth_.setUniformFloat("u_FarPlane", frustum.getFar());
    
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glsl::Program::unbind();
}

void Table::render(const Frustum &frustum, const ConeLight &light, 
        const Texture &shadowMap) {
    const auto view = frustum.getView();

    auto depthBiasProjViewMat = utils::biasMatrix * light.computeProjViewMat();
    auto normalMat = glm::transpose(glm::inverse(glm::mat3(view)));

    program_.bind();
    program_.setUniformMat4("u_ModelviewProjectionMat", false, frustum.getViewProjPtr());
    program_.setUniformMat4("u_ModelviewMat", false, glm::value_ptr(view)); // view is equal to modelView.
    program_.setUniformMat4("u_DepthBiasMat", false, glm::value_ptr(depthBiasProjViewMat));
    program_.setUniformMat3("u_NormalMat", false, glm::value_ptr(normalMat));
    
    program_.setUniformInt("u_ShadowMap", 0);
    program_.setUniformInt("u_Texture", 1);
    light.bind(program_, frustum);
    
    glCullFace(GL_BACK);
    glBindVertexArray(vao_);
    shadowMap.bind<GL_TEXTURE_2D>();
    glActiveTexture(GL_TEXTURE1);
    texture_.bind<GL_TEXTURE_2D>();
    glActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glsl::Program::unbind();
}

}