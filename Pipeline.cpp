#include "stdafx.h"
#include "Pipeline.h"

void Pipeline::program_iterator()
{
	glValidateProgramPipeline(ppo);
	GLint  status;
	glGetProgramPipelineiv(ppo, GL_VALIDATE_STATUS, &status);
	if (GL_FALSE == status)
	{
		GLint length;
		glGetProgramPipelineiv(ppo, GL_INFO_LOG_LENGTH, &length);
		std::string log(length, '\0');
		glGetProgramPipelineInfoLog(ppo, length, NULL, &log[0]);
		throw "Invalid pipeline " + log;
	}
}

const std::array<GLuint, PROGRAM_TYPE::NUM_TYPE> Pipeline::gl_shader_bit = { GL_VERTEX_SHADER_BIT, GL_GEOMETRY_SHADER_BIT, GL_FRAGMENT_SHADER_BIT };

Voxelize::Voxelize(GLsizei size_)
	: Pipeline(PROGRAM_TYPE::VERTEX, "voxel_par_triangle_vs.glsl", PROGRAM_TYPE::GEOMETRY, "voxel_par_triangle_gs.glsl"), 
	size(size_)
{
	// world ([0, 2]^3) to model (grid [0, N)^3) scaling transformation
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_model", size * 0.5f);

	glGenTextures(1, &image); //glCreateTextures(GL_TEXTURE_3D, 1, &grid_image);
	glBindTexture(GL_TEXTURE_3D, image); //glTextureStorage3D(grid_image, 1, GL_R32UI, grid_size, grid_size, grid_size);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, size, size, size); // immutable storage works
	// when <layered> is FALSE, 3D texture is treated as two-dimensional textures
	glBindImageTexture(0, image, 0, /*layered=*/GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
	
	GLuint clear = 0; // *not* internalFormat
	glClearTexImage(image, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear);
	//std::vector<GLuint> pixels(size * size * size, 0);
	////glGetTextureImage(image, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, sizeof(GLuint) * pixels.size(), pixels.data());
	//glGetTexImage(GL_TEXTURE_3D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, pixels.data());
	//assert(pixels[0] == clear);
}

Raycast::Raycast(GLsizei size, GLuint image, const Camera* camera_)
	: Pipeline(PROGRAM_TYPE::VERTEX, "raycasting_vs.glsl", PROGRAM_TYPE::FRAGMENT, "raycasting_fs.glsl"), 
	camera(camera_), inv_model(size * 0.5f)
{
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_view_mat", glm::mat3(camera->get_inv_view()));
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_proj_mat", glm::mat4x2(camera->get_inv_proj()));
	set_uniform(PROGRAM_TYPE::VERTEX, "clip_w", camera->get_zNear());
	set_uniform(PROGRAM_TYPE::FRAGMENT, "eye", camera->get_eye());
	// world ([-1, 1]^3) to model (grid [0, N)^3) translation & scaling transformation
	set_uniform(PROGRAM_TYPE::FRAGMENT, "eye_grid", inv_model * (camera->get_eye() + 1.f));
	set_uniform(PROGRAM_TYPE::FRAGMENT, "inv_model", inv_model);

	glBindImageTexture(0, image, 0, /*layered=*/GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
}

Raycast::Raycast(GLsizei size, const GLuint* buffer, const Camera* camera_)
	: Pipeline(PROGRAM_TYPE::VERTEX, "raycasting_vs.glsl", PROGRAM_TYPE::FRAGMENT, "raycasting_fs.glsl"),
	camera(camera_), inv_model(size * 0.5f)
{
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_view_mat", glm::mat3(camera->get_inv_view()));
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_proj_mat", glm::mat4x2(camera->get_inv_proj()));
	set_uniform(PROGRAM_TYPE::VERTEX, "clip_w", camera->get_zNear());
	set_uniform(PROGRAM_TYPE::FRAGMENT, "eye", camera->get_eye());
	// world ([-1, 1]^3) to model (grid [0, N)^3) translation & scaling transformation
	set_uniform(PROGRAM_TYPE::FRAGMENT, "eye_grid", inv_model * (camera->get_eye() + 1.f));
	set_uniform(PROGRAM_TYPE::FRAGMENT, "inv_model", inv_model);
	set_uniform(PROGRAM_TYPE::FRAGMENT, "size", GLuint(size));
	set_uniform(PROGRAM_TYPE::FRAGMENT, "size3", GLuint(size * size * size));

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LinkedList::BUFFER_TYPE::NEXT, buffer[LinkedList::BUFFER_TYPE::NEXT]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LinkedList::BUFFER_TYPE::COMP, buffer[LinkedList::BUFFER_TYPE::COMP]);
}

LinkedList::LinkedList(GLsizei size_, GLsizei depth_)
	: Pipeline(PROGRAM_TYPE::VERTEX, "voxel_par_triangle_vs.glsl", PROGRAM_TYPE::GEOMETRY, "voxel_par_triangle_gs.glsl"), 
	size(size_), depth(depth_)
{
	// world ([0, 2]^3) to model (grid [0, N)^3) scaling transformation
	set_uniform(PROGRAM_TYPE::VERTEX, "inv_model", size * 0.5f);

	const GLuint size3 = size * size * size;
	glGenBuffers(BUFFER_TYPE::NUM_TYPE, buffer);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffer[BUFFER_TYPE::COUNTER]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &size3, GL_DYNAMIC_DRAW);

	const GLuint uiclear = 0;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_TYPE::NEXT, buffer[BUFFER_TYPE::NEXT]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * size3 * depth, 0, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &uiclear);
	
	const GLint iclear = glm::floatBitsToInt(1e20); // infinity
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_TYPE::COMP, buffer[BUFFER_TYPE::COMP]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint) * size3 * depth, 0, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &iclear);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_TYPE::DATA, buffer[BUFFER_TYPE::DATA]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint) * size3 * (depth - 1), 0, GL_DYNAMIC_DRAW);

	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer[BUFFER_TYPE::NODE]);
	//std::vector<GLuint> data(2 * num_node, 0);
	//glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * data.size(), data.data());

	set_uniform(PROGRAM_TYPE::GEOMETRY, "size", GLuint(size));
	set_uniform(PROGRAM_TYPE::GEOMETRY, "size3", size3);
}

void LinkedList::validate() const
{
	GLuint counter;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer[LinkedList::BUFFER_TYPE::COUNTER]);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &counter);

	const GLuint size3 = size * size * size;
	std::vector<GLuint> next(size3 * depth, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer[LinkedList::BUFFER_TYPE::NEXT]);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * next.size(), next.data());

	std::vector<GLint> comp(size3 * depth, glm::floatBitsToInt(1e20));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer[LinkedList::BUFFER_TYPE::COMP]);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint) * comp.size(), comp.data());

	std::vector<GLint> data(size3 * (depth - 1), 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer[LinkedList::BUFFER_TYPE::DATA]);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint) * data.size(), data.data());

	if (counter > next.size())
	{
		throw "linked list overflow ;)";
	}

	for (size_t i = 0; i < size3; i++)
	{
		const GLfloat abs_min = glm::abs(glm::intBitsToFloat(comp[i]));
		// traverse linked list
		for (GLuint j = next[i]; j != 0; j = next[j])
		{
			if (glm::abs(glm::intBitsToFloat(comp[j])) < abs_min)
			{
				throw "error nearest-to-zero comp value in head T_T";
			}
		}
	}
}
