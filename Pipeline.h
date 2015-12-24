#pragma once
#include "Program.h"
#include "Camera.h"

class Pipeline
{
public:
	template<typename... Ts> // variadic template constructor
	Pipeline(Ts... args)
	{
		glGenProgramPipelines(1, &ppo);

		program_iterator(args...);
	}
	Pipeline(Pipeline&&) = delete; // no default move constructor
	~Pipeline() { glDeleteProgramPipelines(1, &ppo); }

	void bind() const { glBindProgramPipeline(ppo); }
	template<typename T>
	void set_uniform(PROGRAM_TYPE type, const char* name, const T& value) const
	{
		program_array[type].set_uniform(name, value);
	}
private:
	template<typename... Ts>
	void program_iterator(PROGRAM_TYPE type, const char* file_name, Ts... args)
	{
		if (program_array[type].spo != 0)
		{
			throw "Duplicate shader";
		}
		program_array[type] = Program(type, file_name);
		glUseProgramStages(ppo, gl_shader_bit[type], program_array[type].spo);

		program_iterator(args...);
	}
	void program_iterator();

	GLuint ppo;
	static const std::array<GLuint, PROGRAM_TYPE::NUM_TYPE> gl_shader_bit;
	std::array<Program, PROGRAM_TYPE::NUM_TYPE> program_array;
};

class Voxelize : public Pipeline
{
public:
	Voxelize(GLsizei size);
	~Voxelize() { glDeleteTextures(1, &image); }

	GLuint get_image() const { return image; }
private:
	GLuint image;
	GLsizei size;
};

class Raycast : public Pipeline
{
public:
	Raycast(GLsizei size, GLuint image, const Camera* camera);
	Raycast(GLsizei size, const GLuint* buffer, const Camera* camera);
	~Raycast() {}

private:
	float inv_model;
	const Camera* camera;
};

class LinkedList : public Pipeline
{
public:
	LinkedList(GLsizei size, GLsizei depth);
	~LinkedList() { glDeleteBuffers(BUFFER_TYPE::NUM_TYPE, buffer); }

	enum BUFFER_TYPE { NEXT, COMP, DATA, COUNTER, NUM_TYPE };
	const GLuint* get_buffer() const { return buffer; }

	void validate() const;
private:
	GLsizei size;
	GLsizei depth; // average depth, including the dummy head
	GLuint buffer[BUFFER_TYPE::NUM_TYPE];
};