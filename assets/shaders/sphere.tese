layout(triangles, equal_spacing) in;

uniform mat4 u_ModelviewProjectionMat;

#if (!defined(SHADOW_PASS)) && (!defined(DEPTH_PASS))
#define NORMAL_PASS
#endif

#if defined(NORMAL_PASS) || defined(DEPTH_PASS) 
uniform mat4 u_ModelViewMat;
#endif

#ifdef NORMAL_PASS
uniform mat3 u_NormalMat;
#endif

in vec3 v_PositionEval[];

#ifdef NORMAL_PASS
out vec3 v_Tesselated;
out vec3 v_Normal;
out vec3 v_Eye;
#endif

#ifdef DEPTH_PASS
out float v_Depth;
#endif

vec3 computeTesselated() 
{
    return normalize(gl_TessCoord.x * v_PositionEval[0] 
        + gl_TessCoord.y * v_PositionEval[1] 
        + gl_TessCoord.z * v_PositionEval[2]);
}

void main(void)
{
    vec3 tesselated = computeTesselated();

#ifdef NORMAL_PASS
    v_Tesselated = tesselated;
    v_Normal = normalize(u_NormalMat * v_Tesselated);
    v_Eye = vec3(u_ModelViewMat * vec4(v_Tesselated, 1));
#endif

#ifdef DEPTH_PASS
    v_Depth = -(u_ModelViewMat * vec4(tesselated, 1)).z;
#endif
    
    gl_Position = u_ModelviewProjectionMat * vec4(tesselated, 1);
}