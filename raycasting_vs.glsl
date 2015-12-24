#version 450
out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
	float gl_ClipDistance[];
};

layout(location = 0) in vec2 position;
// direction w.r.t. world coordinate, perspective interpolation is unnecessary since
// the quad (i.e. zNear plane) is perpendicular to -z-axis in clip coordinate
noperspective out vec3 ray_dir;

uniform mat3 inv_view_mat; // only the upper 3x3 submatrix
uniform mat4x2 inv_proj_mat; // the first 2 rows of the inverse projection matrix
uniform float clip_w; // i.e. -z_eye = -(-zNear) = zNear of glm::perspective

void main() {
	vec4 position_nd = vec4(position, -1.0, 1.0);
	// world <- eye <- clip <- normalized device, note the eye is at the origin in eye coordinate
	ray_dir = inv_view_mat * vec3(inv_proj_mat * position_nd * clip_w, -clip_w);

	gl_Position = position_nd; 
}