#ifndef DEPTH_PASS
uniform float u_Light0SpotExp;
uniform float u_Light0SpotCosCutoff;
uniform vec3 u_Light0Pos;
uniform vec3 u_Light0SpotDir;
uniform sampler2D u_ShadowMap;
uniform sampler2D u_Texture;
#else
uniform float u_NearPlane;
uniform float u_FarPlane;
#endif

#ifdef DEPTH_PASS
in float v_Depth;
#else
in vec3 v_EyeSpaceNormal;
in vec3 v_EyeSpaceVertex;
in vec2 v_TexCoords;
in vec4 v_ShadowCoords;
#endif

out vec4 color;

#define CONSTANT_ATTENUATION 0.0f
#define LINEAR_ATTENUATION 0.7f
#define QUADRATIC_ATTENUATION 0.4f
#define SHININESS 20

#ifndef DEPTH_PASS
float light0() {
    // no global ambient: want totally black non-illuminated environment.
    float color = 0.0; 

    vec3 L = normalize(u_Light0Pos - v_EyeSpaceVertex);
    vec3 E = normalize(-v_EyeSpaceVertex); // we are in Eye Coordinates, so EyePos is (0,0,0)
    vec3 R = normalize(-reflect(L, v_EyeSpaceNormal)); 
 
    float NdotL = max(dot(v_EyeSpaceNormal, L), 0.0);

    if (NdotL > 0.0) {
        float spotEffect = dot(normalize(u_Light0SpotDir), -L); 
        if (spotEffect > u_Light0SpotCosCutoff) {
            spotEffect = clamp((spotEffect - u_Light0SpotCosCutoff) / (1 - u_Light0SpotCosCutoff), 0, 1) 
                            * min(pow(spotEffect, u_Light0SpotExp), 1.0);

            float dist = length(L);

            float atten = spotEffect / 
                (CONSTANT_ATTENUATION 
                    + LINEAR_ATTENUATION * dist 
                    + QUADRATIC_ATTENUATION * dist * dist);

            //calculate Ambient Term:
            float Iamb = 0.0f; 

            //calculate Diffuse Term:
            float Idiff = NdotL;
 
            // calculate Specular Term:
            float Ispec = 0.25 * pow(max(dot(R,E), 0.0), SHININESS);
            
            color += atten * (Iamb + Idiff + Ispec);
        };
    };
 
    return color;
}

float getVisibility(sampler2D shadowMap, vec4 coords) {
    vec4 vsm = texture(shadowMap, coords.xy);
    float mu = vsm.x;
    float s2 = vsm.y - mu * mu;
    float pmax = s2 / (s2 + (coords.z - mu) * (coords.z - mu));
    return coords.z > vsm.x ? pmax : 1;
}
#endif

void main(void) 
{
#ifdef DEPTH_PASS
    gl_FragDepth = (v_Depth - u_NearPlane) / (u_FarPlane - u_NearPlane);
    color = vec4(1);
#else
    color = vec4(1) 
            * getVisibility(u_ShadowMap, v_ShadowCoords / v_ShadowCoords.w) 
            * light0() 
            * texture(u_Texture, v_TexCoords);
#endif
}