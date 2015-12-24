#pragma once
class Camera
{
public:
	Camera();
	~Camera();

	static const int width = 700, height = 360;

	void lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	void perspective(float fovy, float zNear, float zFar);

	glm::mat4 get_view() const { return view; }
	glm::mat4 get_proj() const { return proj; }
	glm::mat4 get_inv_view() const { return inv_view; }
	glm::mat4 get_inv_proj() const { return inv_proj; }
	glm::vec3 get_eye() const { return eye; }
	float get_zNear() const { return zNear; }
	// since there is no scaling in view, the upper mat3 of the transpose of 
	// the inverse of view mat4 remains unchanged
	glm::mat3 get_normal() const { return glm::mat3(view); }

	void print(const glm::mat4& mat) const;
	bool is_identity(const glm::mat4& mat) const;
private:
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 inv_view;
	glm::mat4 inv_proj;

	glm::vec3 eye;
	float zNear;
};

