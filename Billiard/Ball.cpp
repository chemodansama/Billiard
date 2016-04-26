#include "StdAfx.h"
#include "Ball.h"

#include "utils.h"

#include <cmath>
#include <utility>
#include <cstddef>
#include <unordered_map>

#include "glm\gtx\transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include "glm\gtc\matrix_transform.hpp"

#define USE_GL_TESSELATION 1

template <class T>
inline void hash_combine(std::size_t & seed, const T & v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<> struct std::hash<std::pair<GLushort, GLushort>> {
	inline size_t operator()(const std::pair<GLushort, GLushort> & v) const {
		size_t seed = 0;
		hash_combine(seed, v.first);
		hash_combine(seed, v.second);
		return seed;
	}
};

namespace billiard {

#define X .525731112119133606f
#define Z .850650808352039932f

namespace {
	GLfloat vertices[] = {    
	   -X, 0.0, Z, 
	   X, 0.0, Z, 
	   -X, 0.0, -Z, 
	   X, 0.0, -Z,
	   0.0, Z, X, 
	   0.0, Z, -X, 
	   0.0, -Z, X, 
	   0.0, -Z, -X,
	   Z, X, 0.0, 
	   -Z, X, 0.0, 
	   Z, -X, 0.0, 
	   -Z, -X, 0.0
	};

	GLushort indices[] = { 
	   0,4,1, 
	   0,9,4, 
	   9,5,4, 
	   4,5,8, 
	   4,8,1,    
	   8,10,1, 
	   8,3,10, 
	   5,3,8, 
	   5,2,3, 
	   2,7,3,    
	   7,10,3, 
	   7,6,10, 
	   7,11,6, 
	   11,0,6, 
	   0,1,6, 
	   6,1,10, 
	   9,0,11, 
	   9,11,2, 
	   9,2,5, 
	   7,2,11 };

	std::vector<GLushort> tesselate(std::vector<GLfloat> &vertices, 
									const std::vector<GLushort> &indices) {
		assert(indices.size() % 3 == 0);

		std::vector<GLushort> result;
		std::unordered_map<std::pair<GLushort, GLushort>, GLushort> midindices;

		for (int face = 0, count = indices.size() / 3; face < count; face++) {
			GLushort mid[3];
			for (auto i = 0; i < 3; i++) {
				auto i0_ = indices[face * 3 + i];
				auto i1_ = indices[face * 3 + (i + 1) % 3];
				auto imin = std::min(i0_, i1_);
				auto imax = std::max(i0_, i1_);
				auto key = std::make_pair(imin, imax);
				auto it = midindices.find(key);
				if (it == midindices.end()) {
					auto v0 = glm::vec3(vertices[imin * 3 + 0], vertices[imin * 3 + 1], vertices[imin * 3 + 2]);
					auto v1 = glm::vec3(vertices[imax * 3 + 0], vertices[imax * 3 + 1], vertices[imax * 3 + 2]);
					auto midv = glm::normalize(v0 + (v1 - v0) * 0.5f);
                    mid[i] = static_cast<GLushort>(vertices.size() / 3);
					vertices.push_back(midv.x);
					vertices.push_back(midv.y);
					vertices.push_back(midv.z);
					midindices.insert(std::make_pair(key, mid[i]));
				} else {
					mid[i] = it->second;
				}
			}
			result.push_back(indices[face * 3 + 0]);
			result.push_back(mid[0]);
			result.push_back(mid[2]);

			result.push_back(mid[0]);
			result.push_back(indices[face * 3 + 1]);
			result.push_back(mid[1]);

			result.push_back(mid[2]);
			result.push_back(mid[1]);
			result.push_back(indices[face * 3 + 2]);

			result.push_back(mid[0]);
			result.push_back(mid[1]);
			result.push_back(mid[2]);
		}
		return result;
	}

	void tesselate(std::vector<GLfloat> &vertices, 
				   std::vector<GLushort> &indices,
				   int levels) {
		for (auto i = 0; i < levels / 2; i++) {
			auto newIndices = tesselate(vertices, indices);
			indices = tesselate(vertices, newIndices);
		}
		if (levels % 2) {
			indices = tesselate(vertices, indices);
		}
	}

	std::pair<std::vector<GLfloat>, std::vector<GLushort>> tesselate(
			const GLfloat *vertices, const int verticesLen,
			const GLushort *indices, const int indicesLen, int levels) {
		std::vector<GLfloat> verticesVector(vertices, vertices + verticesLen);
		std::vector<GLushort> indicesVector(indices, indices + indicesLen);
		tesselate(verticesVector, indicesVector, levels);
		return std::make_pair(std::move(verticesVector), std::move(indicesVector));
	}

	void prepareAlbedo(const Texture &albedo, const std::string &filename) {
		albedo.bind<GL_TEXTURE_2D>();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		unsigned int w, h, bpp;
		auto data = utils::loadPng(filename.c_str(), &w, &h, &bpp);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}

    glm::mat4 createModelMat() {
        glm::mat4 modelMat;
        modelMat = glm::scale(modelMat, glm::vec3(BALL_DIAMETER / 2));
        modelMat = glm::translate(modelMat, glm::vec3(0, 0, 1));
        return modelMat;
    }

    void doRender(bool lines, GLuint vao, GLsizei count) {
        glCullFace(GL_FRONT);
        glPolygonMode(GL_FRONT_AND_BACK, lines ? GL_LINE : GL_FILL);

        glBindVertexArray(vao);
        glDrawElements(GL_PATCHES, count, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
    }
}

Ball::Ball(const std::string &exePath)
        : programs_(glsl::loadShaderFromFile(exePath + "../assets/shaders/sphere.vert"), 
#ifdef USE_GL_TESSELATION
                    glsl::loadShaderFromFile(exePath + "../assets/shaders/sphere.tesc"), 
                    glsl::loadShaderFromFile(exePath + "../assets/shaders/sphere.tese"), 
#endif
                    glsl::loadShaderFromFile(exePath + "../assets/shaders/sphere.frag"))
        , data_(tesselate(vertices, utils::length(vertices), indices, utils::length(indices), 2))
        , vbo_(VertexBuffer::create<GL_ARRAY_BUFFER>(data_.first.data(), data_.first.size()))
        , indices_(VertexBuffer::create<GL_ELEMENT_ARRAY_BUFFER>(data_.second.data(), data_.second.size()))
        , lineFill_(false)
        , modelMat_(createModelMat()) {
    glBindVertexArray(vao_);
    vbo_.bind<GL_ARRAY_BUFFER>();
    indices_.bind<GL_ELEMENT_ARRAY_BUFFER>();
    glsl::Program::setAttrPtr(0, 3, 0, nullptr);
    glBindVertexArray(0);
    VertexBuffer::unbind<GL_ARRAY_BUFFER>(); 
    VertexBuffer::unbind<GL_ELEMENT_ARRAY_BUFFER>();

    prepareAlbedo(albedo_, exePath + "../assets/textures/ball_albedo.png");
}

void Ball::renderShadow() const {
    programs_.shadow_.bind();
    doRender(lineFill_, vao_, data_.second.size());
    glsl::Program::unbind();
}

void Ball::render() const {
    albedo_.bind<GL_TEXTURE_2D>();
    programs_.normal_.bind();
    doRender(lineFill_, vao_, data_.second.size());
    glsl::Program::unbind();
}

void Ball::renderDepth() const {
    programs_.depth_.bind();
    doRender(lineFill_, vao_, data_.second.size());
    glsl::Program::unbind();
}

void Ball::update(const Frustum &frustum, const ConeLight &light) {
    const auto &view = frustum.getView();
    auto modelView = view * modelMat_;
    auto modelViewInv = glm::inverse(modelView);
    auto cameraWorldPos = modelViewInv * glm::vec4(0, 0, 0, 1);
    cameraWorldPos /= cameraWorldPos.w;
    
    auto normalMat = glm::transpose(glm::inverse(glm::mat3(modelView)));
    auto modelViewProj = frustum.getProj() * modelView;

    programs_.normal_.bind();
    programs_.normal_.setUniformMat4("u_ModelviewProjectionMat", false, glm::value_ptr(modelViewProj));
    programs_.normal_.setUniformMat4("u_ModelMat", false, glm::value_ptr(modelMat_));
    programs_.normal_.setUniformMat4("u_ModelViewMat", false, glm::value_ptr(modelView));
    programs_.normal_.setUniformMat3("u_NormalMat", false, glm::value_ptr(normalMat));
    programs_.normal_.setUniformVec3("u_CameraWorldPos", glm::value_ptr(cameraWorldPos));
    programs_.normal_.setUniformInt("u_Albedo", 0);
    light.bind(programs_.normal_, frustum);

    depthModelViewProj_ = light.computeProjViewMat() * modelMat_;
    
    programs_.shadow_.bind();
    programs_.shadow_.setUniformMat4("u_ModelMat", false, glm::value_ptr(modelMat_));
    programs_.shadow_.setUniformMat4("u_ModelviewProjectionMat", false, glm::value_ptr(depthModelViewProj_));
    programs_.shadow_.setUniformVec3("u_CameraWorldPos", glm::value_ptr(cameraWorldPos));

    programs_.depth_.bind();
    programs_.depth_.setUniformMat4("u_ModelMat", false, glm::value_ptr(modelMat_));
    programs_.depth_.setUniformMat4("u_ModelViewMat", false, glm::value_ptr(modelView));
    programs_.depth_.setUniformMat4("u_ModelviewProjectionMat", false, glm::value_ptr(modelViewProj));
    programs_.depth_.setUniformVec3("u_CameraWorldPos", glm::value_ptr(cameraWorldPos));
    programs_.depth_.setUniformFloat("u_NearPlane", frustum.getNear());
    programs_.depth_.setUniformFloat("u_FarPlane", frustum.getFar());

    glsl::Program::unbind();
}

void Ball::setLineFill(bool value) {
    lineFill_ = value;
}

}