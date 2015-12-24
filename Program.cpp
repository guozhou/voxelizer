#include "stdafx.h"
#include "Program.h"
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

Program::Program(PROGRAM_TYPE type_, const char* file_name)
{
	type = type_;

	std::ifstream in(file_name, std::ios::in);
	if (!in)
	{
		throw "Cannot open " + std::string(file_name);
	}
	std::ostringstream contents;
	contents << in.rdbuf();
	in.close();
	std::string src_str = contents.str();
	const GLchar* src = src_str.c_str();

	GLint status;
	spo = glCreateShaderProgramv(gl_shader[type], 1, &src);
	glGetProgramiv(spo, GL_LINK_STATUS, &status);
	if (GL_FALSE == status)
	{
		GLint length;
		glGetProgramiv(spo, GL_INFO_LOG_LENGTH, &length);
		std::string log(length, '\0');
		glGetProgramInfoLog(spo, length, NULL, &log[0]);
		throw std::string(file_name) + " failed " + log;
	}
}

void Program::set_uniform(const char* name, const glm::mat4& mat) const
{
	glProgramUniformMatrix4fv(spo, glGetUniformLocation(spo, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Program::set_uniform(const char* name, const glm::mat3& mat) const
{
	glProgramUniformMatrix3fv(spo, glGetUniformLocation(spo, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Program::set_uniform(const char* name, const glm::mat4x2& mat) const
{
	glProgramUniformMatrix4x2fv(spo, glGetUniformLocation(spo, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Program::set_uniform(const char* name, const float& value) const
{
	glProgramUniform1f(spo, glGetUniformLocation(spo, name), value);
}

void Program::set_uniform(const char* name, const glm::vec3& vec) const
{
	glProgramUniform3fv(spo, glGetUniformLocation(spo, name), 1, glm::value_ptr(vec));
}

void Program::set_uniform(const char* name, const GLuint& value) const
{
	glProgramUniform1ui(spo, glGetUniformLocation(spo, name), value);
}

const std::array<GLuint, PROGRAM_TYPE::NUM_TYPE> Program::gl_shader = { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER };