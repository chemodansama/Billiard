uniform mat4 u_ModelMat;

layout(location = 0) in vec3 position;

out vec3 v_WorldPosition;
out vec3 v_PositionControl;

void main(void)
{
    v_PositionControl = position;
    v_WorldPosition = vec3(u_ModelMat * vec4(position, 1.0));
}