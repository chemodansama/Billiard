uniform mat4 u_ModelviewProjectionMat;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

out vec2 v_TexCoords;

void main(void)
{
    v_TexCoords = texCoords;
    gl_Position = u_ModelviewProjectionMat * vec4(position, 1);
}
