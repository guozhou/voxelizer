#version 450
out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
}; // redeclare for ARB_separate_shader_objects

layout(location = 0) in vec3 position; // VERTEX_ATTRIB::POSITION
layout(location = 1) in vec3 normal; // VERTEX_ATTRIB::NORMAL
out vec3 normal_model;

uniform float inv_model; // world to grid coordinate

void main() {
	// assuming the input model is in [-1, 1]^3
	// grid <- world [0, 2]^3 <- world [-1, 1]^3
	normal_model = normal;
	gl_Position = vec4(inv_model * (position + 1.0), 1.0); 
}