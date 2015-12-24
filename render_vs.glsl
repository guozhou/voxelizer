#version 450
out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
}; // redeclare for ARB_separate_shader_objects

layout(location = 0) in vec3 position; // VERTEX_ATTRIB::POSITION
layout(location = 1) in vec3 normal; // VERTEX_ATTRIB::NORMAL
out vec3 position_eye; // interface matching
out vec3 normal_eye;

uniform mat4 model_view_mat;
uniform mat4 model_view_proj_mat;

void main() {
	position_eye = vec3(model_view_mat * vec4(position, 1.0));
	normal_eye = normalize(mat3(model_view_mat) * normal);
	// clip coordinate (*, *, *, -eye.z), eye.z \in [-zNear, -zFar] where zNear and zFar from glm::perspective
	gl_Position = model_view_proj_mat * vec4(position, 1.0);
}