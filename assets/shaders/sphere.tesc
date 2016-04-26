layout (vertices = 3) out;

uniform vec3 u_CameraWorldPos;

in vec3 v_WorldPosition[];
in vec3 v_PositionControl[];

out vec3 v_PositionEval[];

float GetTessLevel(float avgDistance)
{
	return mix(6, 2, clamp(avgDistance / 5, 0, 1));
}

float GetTessLevel(float distance0, float distance1)
{
	return GetTessLevel((distance0 + distance1) / 2.0);
}

void main ()
{                                       
	v_PositionEval[gl_InvocationID] = v_PositionControl[gl_InvocationID];

	if (gl_InvocationID == 0) {
		float eyeToVertexDistance0 = distance(u_CameraWorldPos, v_WorldPosition[0]);
		float eyeToVertexDistance1 = distance(u_CameraWorldPos, v_WorldPosition[1]);

		float eyeToVertexDistance2 = distance(u_CameraWorldPos, v_WorldPosition[2]);

		gl_TessLevelOuter[0] = GetTessLevel(eyeToVertexDistance1, eyeToVertexDistance2);
		gl_TessLevelOuter[1] = GetTessLevel(eyeToVertexDistance2, eyeToVertexDistance0);
		gl_TessLevelOuter[2] = GetTessLevel(eyeToVertexDistance0, eyeToVertexDistance1);

		float levelInner = max(gl_TessLevelOuter[0], max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));
		gl_TessLevelInner[0] = levelInner;

		gl_TessLevelInner[1] = levelInner;
	}
}