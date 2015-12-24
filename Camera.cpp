#include "stdafx.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

Camera::Camera()
{
}


Camera::~Camera()
{
}

void Camera::lookAt(const glm::vec3& eye_, const glm::vec3& center, const glm::vec3& up)
{
	view = glm::lookAt(eye_, center, up); // translation * rotation
	
	inv_view[0][0] = view[0][0];
	inv_view[0][1] = view[1][0]; // transpose for inverse rotation
	inv_view[0][2] = view[2][0];
	inv_view[1][0] = view[0][1];
	inv_view[1][1] = view[1][1];
	inv_view[1][2] = view[2][1];
	inv_view[2][0] = view[0][2];
	inv_view[2][1] = view[1][2];
	inv_view[2][2] = view[2][2];
	inv_view[3][0] = -dot(view[0], view[3]); // minus for inverse translation
	inv_view[3][1] = -dot(view[1], view[3]);
	inv_view[3][2] = -dot(view[2], view[3]);

	assert(is_identity(inv_view * view));

	eye = eye_;
}

void Camera::perspective(float fovy, float zNear_, float zFar)
{
	proj = glm::perspectiveFov(fovy, float(width), float(height), zNear_, zFar);
	
	inv_proj[0][0] = 1.f / proj[0][0];
	inv_proj[1][1] = 1.f / proj[1][1];
	inv_proj[2][2] = 0.f;
	inv_proj[2][3] = 1.f / proj[3][2];
	inv_proj[3][0] = inv_proj[0][0] * proj[2][0];
	inv_proj[3][1] = inv_proj[1][1] * proj[2][1];
	inv_proj[3][2] = -1.f;
	inv_proj[3][3] = inv_proj[2][3] * proj[2][2];

	assert(is_identity(inv_proj * proj));

	zNear = zNear_;
}

void Camera::print(const glm::mat4& mat) const
{
	printf("mat4(\n");
	printf("\t(%4.4f, %4.4f, %4.4f, %4.4f)\n", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
	printf("\t(%4.4f, %4.4f, %4.4f, %4.4f)\n", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
	printf("\t(%4.4f, %4.4f, %4.4f, %4.4f)\n", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
	printf("\t(%4.4f, %4.4f, %4.4f, %4.4f))\n\n", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

bool Camera::is_identity(const glm::mat4& mat) const
{
	static const glm::vec4 epsilon = glm::vec4(10.f * std::numeric_limits<float>::epsilon());
	glm::mat4 diff = mat - glm::mat4();
	glm::bvec4 mask = 
		glm::lessThanEqual(glm::abs(diff[0]), epsilon) &
		glm::lessThanEqual(glm::abs(diff[1]), epsilon) &
		glm::lessThanEqual(glm::abs(diff[2]), epsilon) &
		glm::lessThanEqual(glm::abs(diff[3]), epsilon);

	return glm::all(mask);
}
