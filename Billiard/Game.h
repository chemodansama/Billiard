#pragma once

#include <string>
#include <GL\glew.h>
#include <GL\GL.h>
#include <glm\glm.hpp>

#include "GlslProgram.h"
#include "Frustum.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "ConeLight.h"

#include "Table.h"
#include "Ball.h"
#include "Particles.h"

namespace billiard {

class LightShaftGeometry {
    static const int SHAFT_PLANES_COUNT = 32;

    VertexArray vao_;
    VertexBuffer vbo_;
    VertexBuffer indices_;

    GLsizei size_;
public:
    LightShaftGeometry();
    void render() const;
};

class Game {
    const std::string exePath_;

    // window size
    int surfaceWidth_;
    int surfaceHeight_;

    bool mouseDown_;
    glm::vec2 mousePos_;

    // frustum related
    glm::vec2 cameraRot_;
    float cameraDistance_;
    Frustum frustum_;

    // scene objects
    Table table_;
    Ball ball_;
    ConeLight light_;

    // scene depth from camera view
    Texture sceneDepthMap_;
    Renderbuffer sceneRenderbuffer_;
    Framebuffer sceneDepthBuffer_;

    // shadow specific
    Texture depthMap_;
    Texture colorMap_; // vsm map 
    Framebuffer shadowBuffer_;

    Texture depthMap2_; // for blurring 
    Texture colorMap2_;
    Framebuffer shadowBuffer2_;

    glsl::Program blurVertically_;
    glsl::Program blurHorizontally_;

    // lightshaft specific
    glsl::Program lightshaft_;
    const LightShaftGeometry lighshaftGeometry_;
    const Texture cookie_;

    VertexBuffer quad_;
    const VertexArray quadVao_;

    void updateProjection();
    void updateModelview();

    void renderShadowMap();
    void renderSceneDepth();
    void renderLightshaft();

    void update();
public:
    Game(int surfaceWidth, int surfaceHeight);
    Game(const Game&);// = delete;

    void render();
    void resize(int surfaceWidth, int surfaceHeight);

    void mouseMoved(float x, float y);
    void mouseDown(float x, float y);
    void mouseUp();
    void mouseScrolled(float y);

	void keyAction(int key, bool pressed);
};

}