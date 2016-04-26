uniform mat4 u_ModelviewProjectionMat;
uniform mat4 u_ModelviewMat;
#ifndef DEPTH_PASS
uniform mat3 u_NormalMat;
uniform mat4 u_DepthBiasMat;
#endif

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

#ifdef DEPTH_PASS
out float v_Depth;
#else
out vec3 v_EyeSpaceNormal;
out vec3 v_EyeSpaceVertex;
out vec2 v_TexCoords;
out vec4 v_ShadowCoords;
#endif

void main(void)
{
#ifdef DEPTH_PASS
    v_Depth = -(u_ModelviewMat * vec4(position, 1)).z;
#else
    v_EyeSpaceVertex = vec3(u_ModelviewMat * vec4(position, 1.0));
    v_EyeSpaceNormal = u_NormalMat * normal;
    v_TexCoords = texCoords;
    v_ShadowCoords = u_DepthBiasMat * vec4(position, 1);
#endif
    gl_Position = u_ModelviewProjectionMat * vec4(position, 1.0);
}