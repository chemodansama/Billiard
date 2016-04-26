uniform mat4 u_ModelviewProjectionMat;

layout(location = 0) in vec3 position;

void main(void)
{
    gl_Position = u_ModelviewProjectionMat * vec4 (position, 1.0);
}