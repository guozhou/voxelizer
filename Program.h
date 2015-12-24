#pragma once

enum PROGRAM_TYPE { VERTEX, GEOMETRY, FRAGMENT, NUM_TYPE };
class Program
{
public:
	Program() { spo = 0; }; // indicates unused program
	Program(PROGRAM_TYPE type, const char* file_name);
	Program(Program&&) = delete; // no default move constructor
	~Program()
	{
		glDeleteProgram(spo);
	}
	Program& operator=(Program& other)
	{
		spo = other.spo;
		type = other.type;
		other.spo = 0;

		return *this;
	}

	void set_uniform(const char* name, const glm::mat4& mat) const;
	void set_uniform(const char* name, const glm::mat3& mat) const;
	void set_uniform(const char* name, const glm::mat4x2& mat) const;
	void set_uniform(const char* name, const float& value) const;
	void set_uniform(const char* name, const glm::vec3& vec) const;
	void set_uniform(const char* name, const GLuint& value) const;

	friend class Pipeline;
private:
	GLuint spo;
	static const std::array<GLuint, PROGRAM_TYPE::NUM_TYPE> gl_shader;
	PROGRAM_TYPE type;
};