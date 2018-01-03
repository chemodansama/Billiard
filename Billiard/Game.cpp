#include "StdAfx.h"
#include "Game.h"

#include <vector>

#include "glm\gtc\matrix_transform.hpp"
#include "glog\logging.h"

#include "utils.h"
#include "Texture.h"
#include "Framebuffer.h"

#define checkError if (auto err = glGetError()) { utils::printStack(); LOG(ERROR) << err; };

namespace billiard {

namespace {
    int shadowMapSize = 1024;
    float shadowMapSizef = static_cast<float>(shadowMapSize);
    float frustumFar = 20.0f;
    
    const float vertices[] =  {
        0, 0, 0, 0, 0,
        shadowMapSizef, 0, 0, 1, 0,
        shadowMapSizef, shadowMapSizef, 0, 1, 1,
        0, shadowMapSizef, 0, 0, 1
    };

    Texture createColorMap(GLenum intFormat, GLenum format, GLenum type, int w, int h) {

        checkError;

        Texture t;
        t.bind<GL_TEXTURE_2D>();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, intFormat, w, h, 0, format, type, nullptr);

        checkError;

        return std::move(t);
    }

    template <GLenum target>
    Texture createDepthMap(GLenum intFormat, GLenum format, GLenum type, int w, int h) {

        checkError;

        Texture t;
        t.bind<target>();

        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        
        glTexImage2D(target, 0, intFormat, w, h, 0, format, type, nullptr);

        checkError;

        return std::move(t);
    }

    void prepareCookie(const Texture &cookie, const char *filename) {
        cookie.bind<GL_TEXTURE_2D>();
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        unsigned int w, h, bpp;
        auto data = utils::loadPng(filename, &w, &h, &bpp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Renderbuffer createSceneRenderbuffer(int w, int h) {
        Renderbuffer rb;
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return rb;
    }

    Framebuffer createFramebuffer(GLuint colorMap, GLuint depthMap, GLuint stencil = 0, GLuint renderBuffer = 0) {
        Framebuffer fb;
        fb.bind<GL_FRAMEBUFFER>();
        if (colorMap > 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorMap, 0);
        } else if (renderBuffer > 0) {
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBuffer);
        } else {
            throw std::runtime_error("Either colorMap or renderBuffer must be specified");
        }

        if (depthMap > 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        }

        if (stencil > 0) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencil, 0);
        }

        if (!isFramebufferOk(glCheckFramebufferStatus(GL_FRAMEBUFFER))) {
            utils::printStack();
            throw std::runtime_error("framebuffer failed");
        }

        Framebuffer::unbind<GL_FRAMEBUFFER>();
        return fb;
    }

    void calcConeXY(const glm::vec3 dir, glm::vec3 *x, glm::vec3 *y) {
        assert(x);
        assert(y);

        const float maxDotProduct = 0.8f;
        const glm::vec3 i (1.0f, 0, 0);
        const glm::vec3 j (0, 1.0f, 0);
        const glm::vec3 k (0, 0, 1.0f);

        *y = glm::dot(i, dir) < maxDotProduct ? i :
             glm::dot(j, dir) < maxDotProduct ? j : k;

        *x = glm::cross(dir, *y);
        *y = glm::cross(*x, dir);

        *x = glm::normalize(*x);
        *y = glm::normalize(*y);
    }

    /* A, B lies in the plane,
    Y - coplanar with the plane,
    P is in a plane's positive half-space */
    glm::vec4 calcPlane(const glm::vec3 &a, const glm::vec3 &b,
            const glm::vec3 &p, const glm::vec3 &y) {
        auto n = glm::normalize(glm::cross(b - a, y));
        auto d = -glm::dot(a, n);

        glm::vec4 result(n, d);
        return glm::dot(n, p) + d > 0 ? result : -result;
    }

    template <int N>
    void calcConeFrustum(const glm::vec3 &a, const glm::vec3 &dir,
            const float tanPhi, const float height, glm::vec4 (&frustum)[N]) {
        static_assert(N == 6, "ensure frustum size");
        frustum[0] = glm::vec4(dir, -glm::dot(a + dir * 0.5f, dir));
        
        // S - center of the base of the cone 
        auto s = a + dir * height * 1.1f;
        frustum[1] = glm::vec4(-dir, -glm::dot(s, -dir));

        glm::vec3 x, y;
        calcConeXY(dir, &x, &y);

        auto r = height * tanPhi;

        // Left & Right eqs 

        frustum[2] = calcPlane(a, s + x * r, s, y);
        frustum[3] = calcPlane(a, s - x * r, s, y);

        // Bottom & Top eqs 

        frustum[4] = calcPlane(a, s + y * r, s, x);
        frustum[5] = calcPlane(a, s - y * r, s, x);
    }

    void detectMinMax(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c,
            float af, float bf, float cf, glm::vec3 *min, float *minf, float *maxf) {
        if (af <= bf) {
            if (bf <= cf) {
              *min = a;
              *maxf = cf;
              *minf = af;
            } else {
                *maxf = bf;
                if (af <= cf) {
                    *min = a;
                    *minf = af;
                } else {
                    *min = c;
                    *minf = cf;
                }
            }
        } else if (bf >= cf) {
            *maxf = af;
            *minf = cf;
            *min = c;
        } else {
            *min = b;
            *minf = bf;
            *maxf = af <= cf ? *maxf = cf : *maxf = af;
        }
    }
}

Game::Game(int surfaceWidth, int surfaceHeight) 
        : exePath_(utils::getExePath())
        , surfaceWidth_(surfaceWidth)
        , surfaceHeight_(surfaceHeight)
        , cameraRot_(0, -60)
        , cameraDistance_(-2.5f)
        , mouseDown_(false)
        , table_(exePath_)
        , ball_(exePath_)
        , sceneDepthMap_(createDepthMap<GL_TEXTURE_RECTANGLE>(GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, surfaceWidth, surfaceHeight))
        , sceneRenderbuffer_(createSceneRenderbuffer(surfaceWidth, surfaceHeight))
        //, sceneDepthMap_(createDepthMap<GL_TEXTURE_RECTANGLE>(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, surfaceWidth, surfaceHeight))
        , sceneDepthBuffer_(createFramebuffer(0, sceneDepthMap_, sceneDepthMap_, sceneRenderbuffer_))
        , depthMap_(createDepthMap<GL_TEXTURE_2D>(GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, shadowMapSize, shadowMapSize))
        , colorMap_(createColorMap(GL_RGBA32F, GL_RGBA, GL_UNSIGNED_INT, shadowMapSize, shadowMapSize))
        , shadowBuffer_(createFramebuffer(colorMap_, depthMap_, depthMap_))
        , depthMap2_(createDepthMap<GL_TEXTURE_2D>(GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, shadowMapSize, shadowMapSize))
        , colorMap2_(createColorMap(GL_RGBA32F, GL_RGBA, GL_UNSIGNED_INT, shadowMapSize, shadowMapSize))
        , shadowBuffer2_(createFramebuffer(colorMap2_, depthMap2_, depthMap2_))
        , blurVertically_("#define BLUR_VERTICALLY\n", 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/blur.vert"), 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/blur.frag"))
        , blurHorizontally_("#define BLUR_HORIZONTALLY\n", 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/blur.vert"), 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/blur.frag"))
        , lightshaft_("", 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/shaft.vert"), 
                   glsl::loadShaderFromFile(exePath_ + "../assets/shaders/shaft.frag"))
        , quad_(VertexBuffer::create<GL_ARRAY_BUFFER>(vertices, utils::length(vertices))) 
        
{
    updateModelview();
    updateProjection();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    light_.setPosition(glm::vec3(0, 2, 2));
    light_.setDirection(glm::normalize(glm::vec3(0, -2, -2)));

    glBindVertexArray(quadVao_);
    quad_.bind<GL_ARRAY_BUFFER>();
    glsl::Program::setAttrPtr(0, 3, 5 * sizeof(float), nullptr);
    glsl::Program::setAttrPtr(1, 2, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    VertexBuffer::unbind<GL_ARRAY_BUFFER>();
    
    auto proj = glm::ortho<float>(0, shadowMapSizef, 0, shadowMapSizef, -1, 1);

    blurVertically_.bind();
    blurVertically_.setUniformInt("u_Texture", 0);
    blurVertically_.setUniformMat4("u_ModelviewProjectionMat", false, glm::value_ptr(proj));

    blurHorizontally_.bind();
    blurHorizontally_.setUniformInt("u_Texture", 0);
    blurHorizontally_.setUniformMat4("u_ModelviewProjectionMat", false, glm::value_ptr(proj));
    
    glsl::Program::unbind();

    prepareCookie(cookie_, (exePath_ + "../assets/textures/cookie.png").c_str());
}

void Game::renderShadowMap() {
    // render shadowmap.
    shadowBuffer_.bind<GL_FRAMEBUFFER>();
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ball_.renderShadow();
    glFlush();

    glBindVertexArray(quadVao_);
    glCullFace(GL_BACK);

    // blur vertically
    shadowBuffer2_.bind<GL_FRAMEBUFFER>();
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    blurVertically_.bind();
    colorMap_.bind<GL_TEXTURE_2D>();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // blur horizontally
    shadowBuffer_.bind<GL_FRAMEBUFFER>();
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    blurHorizontally_.bind();
    colorMap2_.bind<GL_TEXTURE_2D>();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glsl::Program::unbind();
    Framebuffer::unbind<GL_FRAMEBUFFER>();
    glViewport(0, 0, surfaceWidth_, surfaceHeight_);
    glClearColor(0, 0, 0, 1);
}

void Game::update() {
    glm::vec4 lightFrustum[6];
    calcConeFrustum(light_.pos(), light_.dir(), light_.getTanPhi(),
        light_.length(), lightFrustum);

    lightshaft_.bind();
    int loc = lightshaft_.getUniformLocation("u_ClipPlanes[0]");
    for (int i = 0; i < 6; i++) {
        lightshaft_.setUniformVec4(loc + i, glm::value_ptr(lightFrustum[i]));
    } 
    glsl::Program::unbind();
}

void Game::render() {
    update();

    ball_.update(frustum_, light_);
    renderSceneDepth();
    renderShadowMap();

    // render main scene
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    table_.render(frustum_, light_, colorMap_);
    ball_.render();

    renderLightshaft();
}

void Game::resize(int surfaceWidth, int surfaceHeight) {
    surfaceWidth_ = surfaceWidth;
    surfaceHeight_ = surfaceHeight;

    glViewport(0, 0, surfaceWidth, surfaceHeight);
    updateProjection();

    // recreate scene depth framebuffer with new surface size
    //sceneDepthMap_ = createDepthMap<GL_TEXTURE_RECTANGLE>(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, surfaceWidth, surfaceHeight);
    sceneDepthMap_ = createDepthMap<GL_TEXTURE_RECTANGLE>(GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, surfaceWidth, surfaceHeight);
    sceneRenderbuffer_ = createSceneRenderbuffer(surfaceWidth, surfaceHeight);
    sceneDepthBuffer_ = createFramebuffer(0, sceneDepthMap_, sceneDepthMap_, sceneRenderbuffer_);
}

void Game::mouseDown(float x, float y) {
    mousePos_ = glm::vec2(x, y);
    mouseDown_ = true;
}

void Game::mouseUp() {
    mouseDown_ = false;
}

void Game::mouseMoved(float x, float y) {
    auto newPos = glm::vec2(x, y);
    cameraRot_ -= (mousePos_ - newPos) * 0.25f;
    if (cameraRot_.y < -90) {
        cameraRot_.y = -90;
    } else if (cameraRot_.y > 0) {
        cameraRot_.y = 0;
    }

    mousePos_ = newPos;
    updateModelview();
}

void Game::mouseScrolled(float y) {
    cameraDistance_ *= 1 + y / 25;
    if (cameraDistance_ < -8) {
        cameraDistance_ = -8;
    } else if (cameraDistance_ > -1.0f) {
        cameraDistance_ = -1.0f;
    }

    updateModelview();
}

void Game::updateModelview() {
    frustum_.ViewSetIdentity();
    frustum_.ViewTranslate(glm::vec3(0, 0, cameraDistance_));
    frustum_.ViewRotate(cameraRot_.y, glm::vec3(1, 0, 0));
    frustum_.ViewRotate(cameraRot_.x, glm::vec3(0, 0, 1));
    frustum_.ViewTranslate(glm::vec3(0, 0, -BALL_DIAMETER / 2.0));
}

void Game::updateProjection() {
    auto aspect = static_cast<float>(surfaceWidth_) / surfaceHeight_;
    frustum_.ProjSetPerspective(45.0f, aspect, 0.1f, frustumFar);
}

void Game::keyAction(int key, bool pressed) {
    ball_.setLineFill(pressed);
}

void Game::renderSceneDepth() {
    sceneDepthBuffer_.bind<GL_FRAMEBUFFER>();
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    table_.renderDepth(frustum_);
    ball_.renderDepth();
    glFlush();

    glBindVertexArray(0);
    Framebuffer::unbind<GL_FRAMEBUFFER>();
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Game::renderLightshaft() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render dust here.

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glDisable(GL_CULL_FACE);

    auto a = light_.pos();
    auto s = a + light_.dir() * light_.length();

    const auto &view = frustum_.getView();
    auto inverseView = glm::inverse(view);
    auto inverseRotView = glm::inverse(glm::mat3(view));

    auto camPos = inverseView * glm::vec4(0, 0, 0, 1);
    camPos /= camPos.w;

    auto v = glm::normalize(s - glm::vec3(camPos));
    auto p =  glm::cross(std::abs(glm::dot(light_.dir(), v) < 0.999) ? light_.dir() : glm::vec3(1, 0, 0), v);

    auto viewProj = glm::normalize(glm::cross(p, light_.dir()));
    auto r = light_.length() * light_.getTanPhi();

    auto b = s - viewProj * r;
    auto c = s + viewProj * r;

    auto viewZ = -glm::vec3(inverseView[2]);
    auto af = glm::dot(viewZ, a - glm::vec3(camPos));
    auto bf = glm::dot(viewZ, b - glm::vec3(camPos));
    auto cf = glm::dot(viewZ, c - glm::vec3(camPos));
    glm::vec3 min;
    float minf, maxf;
    detectMinMax(a, b, c, af, bf, cf, &min, &minf, &maxf);

    auto eyeLightPos = view * glm::vec4(light_.pos(), 1.0f);
    eyeLightPos /= eyeLightPos.w;

    auto depthBiasProjViewMat = utils::biasMatrix * light_.computeProjViewMat();

    colorMap_.bind<GL_TEXTURE_2D>();
    glActiveTexture(GL_TEXTURE1);
    cookie_.bind<GL_TEXTURE_2D>();
    glActiveTexture(GL_TEXTURE2);
    sceneDepthMap_.bind<GL_TEXTURE_RECTANGLE>();
    glActiveTexture(GL_TEXTURE0);

    lightshaft_.bind();
    lightshaft_.setUniformMat4("u_DepthBiasMat", false, glm::value_ptr(depthBiasProjViewMat));
    
    lightshaft_.setUniformMat3("u_InverseViewRotMat", false, glm::value_ptr(inverseRotView));
    lightshaft_.setUniformMat4("u_ModelViewMat", false, glm::value_ptr(view));
    lightshaft_.setUniformMat4("u_ProjectionMat", false, frustum_.getProjPtr());

    lightshaft_.setUniformVec3("u_ConePos", glm::value_ptr(eyeLightPos));
    lightshaft_.setUniformInt("u_ShadowMap", 0);
    lightshaft_.setUniformInt("u_Texture", 1);
    lightshaft_.setUniformInt("u_Depth", 2);
    lightshaft_.setUniformFloat("u_ConeHeight", light_.length());
    lightshaft_.setUniformFloat("u_TanPhi", light_.getTanPhi());
    lightshaft_.setUniformVec3("u_ConeMin", glm::value_ptr(min));
    lightshaft_.setUniformFloat("u_ConeDepth", std::fabs(maxf - minf));
    
    lightshaft_.setUniformFloat("u_NearPlane", frustum_.getNear());
    lightshaft_.setUniformFloat("u_FarPlane", frustum_.getFar());

    light_.bind(lightshaft_, frustum_);

    for (int i = 0; i < 6; i++) {
        glEnable(GL_CLIP_DISTANCE0 + i);
    }

    glDisable(GL_CULL_FACE);

    lighshaftGeometry_.render();

    glsl::Program::unbind();

    for (int i = 0; i < 6; i++) {
        glDisable(GL_CLIP_PLANE0 + i);
    }
    glDisable(GL_BLEND);
}

LightShaftGeometry::LightShaftGeometry() {
    std::vector<glm::vec3> vertices;
    std::vector<GLushort> indices;
    GLushort index = 0;
    for (int i = SHAFT_PLANES_COUNT - 1; i >= 0; i--) {
        auto slice = static_cast<float>(i) / (SHAFT_PLANES_COUNT - 1);
        vertices.push_back(glm::vec3(-1, -1, slice));
        vertices.push_back(glm::vec3( 1, -1, slice));
        vertices.push_back(glm::vec3( 1,  1, slice));
        vertices.push_back(glm::vec3(-1,  1, slice));

        indices.push_back(index);
        indices.push_back(index + 1);
        indices.push_back(index + 2);

        indices.push_back(index);
        indices.push_back(index + 2);
        indices.push_back(index + 3);
        
        index += 4;
    }
    size_ = indices.size();
    
    glBindVertexArray(vao_);
    vbo_.bind<GL_ARRAY_BUFFER>();
    vbo_.bufferData<GL_ARRAY_BUFFER>(vertices.data(), vertices.size());

    indices_.bind<GL_ELEMENT_ARRAY_BUFFER>();
    indices_.bufferData<GL_ELEMENT_ARRAY_BUFFER>(indices.data(), indices.size());
    
    glsl::Program::setAttrPtr(0, 3, 0, nullptr);
    glBindVertexArray(0);

    VertexBuffer::unbind<GL_ARRAY_BUFFER>(); 
    VertexBuffer::unbind<GL_ELEMENT_ARRAY_BUFFER>();
}

void LightShaftGeometry::render() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, size_, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

}