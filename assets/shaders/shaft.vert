uniform mat4 u_DepthBiasMat;
uniform mat3 u_InverseViewRotMat;
uniform mat4 u_ModelViewMat;
uniform mat4 u_ProjectionMat;

uniform float u_ConeHeight;

uniform vec3 u_ConeMin;
uniform float u_ConeDepth;

uniform vec4 u_ClipPlanes[6];

layout(location = 0) in vec3 position;

out float gl_ClipDistance[6];
out vec3 v_EyeVertex;
out vec4 v_ShadowCoord;

float computeClipDistance(vec4 plane, vec3 vertex) {
    return plane.x * vertex.x + plane.y * vertex.y + plane.z * vertex.z + plane.w;
}

void main()
{
    vec4 vertex = vec4(u_ConeMin
                 + position.x * u_InverseViewRotMat[0] * u_ConeHeight * 2
                 + position.y * u_InverseViewRotMat[1] * u_ConeHeight * 2
                 - position.z * u_InverseViewRotMat[2] * u_ConeDepth, 1);

    vec4 eyePos = u_ModelViewMat * vertex;
    v_EyeVertex = vec3(eyePos);
    
    v_ShadowCoord = u_DepthBiasMat * vertex;
    gl_Position = u_ProjectionMat * eyePos;

    for (int i = 0; i < 6; i++) {
        gl_ClipDistance[i] = computeClipDistance(u_ClipPlanes[i], vertex.xyz);
    }
}