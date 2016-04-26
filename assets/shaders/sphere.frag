#if (!defined(SHADOW_PASS)) && (!defined(DEPTH_PASS))
#define NORMAL_PASS
#endif

#ifdef NORMAL_PASS
uniform sampler2D u_Albedo;
uniform vec3 u_Light0Pos;
#endif

#ifdef DEPTH_PASS
uniform float u_NearPlane;
uniform float u_FarPlane;
#endif

out vec4 color;

#ifdef NORMAL_PASS
in vec3 v_Tesselated;
in vec3 v_Normal;
in vec3 v_Eye;
#endif

#ifdef DEPTH_PASS
in float v_Depth;
#endif

#define M_PI 3.14159265359
#define AMBIENT 0.2
#define DIFFUSE 0.9
#define SPECULAR 0.7
#define SHININESS 30

#ifdef NORMAL_PASS
void computeLighting(out float ambient, out float diffuse, out float spec) 
{
  ambient = 1;

  vec3 s = normalize(u_Light0Pos - v_Eye);
  diffuse = max(dot(s, v_Normal), 0);
  
  vec3 r = reflect(-s, v_Normal);
  spec = diffuse > 0.0 
         ? pow(max(dot(r, normalize(-v_Eye)), 0.0), SHININESS) 
         : 0;
}
#endif

void main(void) 
{
#ifdef SHADOW_PASS
  float depth = gl_FragCoord.z;
  color = vec4(depth, depth * depth, depth, 1);
#elif defined(DEPTH_PASS)
  gl_FragDepth = (v_Depth - u_NearPlane) / (u_FarPlane - u_NearPlane);
  color = vec4(1); // not needed, fragment shader may be empty.
#else
  float ambient, diffuse, spec;
  computeLighting(ambient, diffuse, spec);

  vec2 albedoTexCoord = vec2(clamp(0.5 + atan(v_Tesselated.y, v_Tesselated.x) / (2 * M_PI), 0, 1), 
                             clamp(0.5 + asin(clamp(v_Tesselated.z, -1, 1)) / M_PI, 0, 1));
  color = texture2D(u_Albedo, albedoTexCoord) * (AMBIENT * ambient + DIFFUSE * diffuse + SPECULAR * spec);
#endif
}