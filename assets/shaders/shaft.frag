uniform vec3  u_ConePos;
uniform float u_ConeHeight;

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap;
uniform sampler2DRect u_Depth;
uniform float u_TanPhi;

uniform float u_Light0SpotExp;
uniform float u_Light0SpotCosCutoff;
uniform vec3 u_Light0Pos;
uniform vec3 u_Light0SpotDir;

uniform float u_NearPlane;
uniform float u_FarPlane;

out vec4 color;

in vec3 v_EyeVertex;
in vec4 v_ShadowCoord;

#define M_E 2.71828
#define M_PI 3.14159265

float getVisibility(sampler2D shadowMap, vec4 coords) {
    vec4 vsm = texture(shadowMap, coords.xy);
    float mu = vsm.x;
    float s2 = vsm.y - mu * mu;
    float pmax = s2 / (s2 + (coords.z - mu) * (coords.z - mu));

    return coords.z > vsm.x ? pmax : 1;
}

void main()
{
    vec3 L = normalize(u_Light0Pos - v_EyeVertex);
    float spotEffect = dot(normalize(u_Light0SpotDir), -L); 
    if (spotEffect > u_Light0SpotCosCutoff) {
        spotEffect = clamp((spotEffect - u_Light0SpotCosCutoff) / (1 - u_Light0SpotCosCutoff), 0, 1) 
                            * min(pow(spotEffect, u_Light0SpotExp), 1.0f);
    }

    vec4 shadowCoord = v_ShadowCoord / v_ShadowCoord.w;
    float shadow = getVisibility(u_ShadowMap, shadowCoord);

    float dist = clamp(distance(v_EyeVertex, u_ConePos) / u_ConeHeight, 0.0f, 1.0f);
    float R = dist * u_TanPhi;
    float alpha = 0.25f * spotEffect / (2.0f * R + 1.0f);

    float zs = texture(u_Depth, gl_FragCoord.xy).r;
    float z = (-v_EyeVertex.z - u_NearPlane) / (u_FarPlane - u_NearPlane);
    float dz = smoothstep(0.0f, 0.01f, zs - z);

    color = vec4(vec3(shadow), dz * alpha) * texture(u_Texture, shadowCoord.xy).r;
}