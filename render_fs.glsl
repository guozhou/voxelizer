#version 450
in vec3 position_eye;
in vec3 normal_eye;
layout(location = 0) out vec4 frag_color;

void main() {
	frag_color = vec4(clamp(dot(normalize(-position_eye), normal_eye), 0.0, 1.0));
}