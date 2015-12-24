// voxel.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Mesh.h"
#include "Program.h"
#include "Pipeline.h"
#include "Camera.h"
#include <glm/gtx/rotate_vector.hpp>

struct UserContext 
{
	Camera* const camera;
	Pipeline* const pipeline;
};

#ifdef _DEBUG
// __stdcall is necessary to override default calling convention __cdecl of MSVC
static void __stdcall gl_debug_cb(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	fprintf(stderr, "(ARB_debug)%s\n", message);
}
#endif

static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static void cursor_pos_cb(GLFWwindow* window, double x, double y)
{
	// arcball
	static bool state = false;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		// orthogonal projection from window coordinate (MFC) to an unit sphere
		auto project = [](const glm::vec2& xy) {
			glm::vec3 p(0.f);
			// window_size -> [-1, 1]^2
			p.xy = glm::clamp(xy / glm::vec2(Camera::width, Camera::height) * 2.f - 1.f, -1.f, 1.f);
			p.y = -p.y;

			float p_len = glm::length(p);
			if (p_len >= 1.f)
			{
				p /= p_len; // form outside onto the unit circle
			}
			else
			{
				p.z = glm::sqrt(1.f - p_len);
			}

			return p;
		};
		// previous sphere projection
		static glm::vec3 p0;
		// the 1st time the left button is pressed
		if (!state)
		{
			p0 = project(glm::vec2(x, y));
			state = true;

			return;
		}
		// the 2nd time the left button is still pressed
		static glm::mat3 total_rotation;
		glm::vec3 p = project(glm::vec2(x, y));
		// walk from p0 to p on the unit sphere
		float angle = glm::acos(glm::dot(p0, p));
		assert(!std::isnan(angle));
		glm::vec3 axis = glm::cross(p0, p);
		glm::mat3 rotation(glm::rotate(glm::degrees(angle), axis));
		total_rotation = rotation * total_rotation;
		glm::mat3 inv_total_rotation = glm::transpose(total_rotation);
		// x, y are relative to the upper-left corner of the client area of the window
		UserContext* userContext = static_cast<UserContext*>(glfwGetWindowUserPointer(window));
		userContext->pipeline->set_uniform(PROGRAM_TYPE::VERTEX, "view_mat",
			glm::mat3(userContext->camera->get_view()) * total_rotation);
		userContext->pipeline->set_uniform(PROGRAM_TYPE::VERTEX, "inv_view_mat",
			inv_total_rotation * glm::mat3(userContext->camera->get_inv_view()));
		userContext->pipeline->set_uniform(PROGRAM_TYPE::FRAGMENT, "eye",
			inv_total_rotation * userContext->camera->get_eye());

		p0 = p;
	}
	else
	{
		state = false;
	}
}

GLFWwindow* init_ogl()
{
	GLFWwindow* window;
	if (!glfwInit())
		throw "glfwInit failed";
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	window = glfwCreateWindow(Camera::width, Camera::height, "Voxel", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		throw "glfwCreateWindow failed";
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_cb);
	//glfwSetCursorPosCallback(window, cursor_pos_cb);

	GLenum err = glewInit(); // after the OGL context is available
	if (GLEW_OK != err)
	{
		glfwTerminate();
		throw "glewInit failed " + std::string((const char *)glewGetErrorString(err));
	}
	fprintf(stdout, "GLEW %s\n", glewGetString(GLEW_VERSION));

#ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(
		(GLDEBUGPROC)&gl_debug_cb, NULL);
#endif

	return window;
}

int _tmain(int argc, _TCHAR* argv[])
{
	try
	{
		GLFWwindow* window = init_ogl();
		
		Camera camera;
		camera.lookAt(glm::vec3(0.8f, 1.f, -1.f), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
		camera.perspective(glm::quarter_pi<float>(), 0.1f, 4.f);

		//Pipeline pipeline(PROGRAM_TYPE::VERTEX, "render_vs.glsl", PROGRAM_TYPE::FRAGMENT, "render_fs.glsl");
		//pipeline.set_uniform(PROGRAM_TYPE::VERTEX, "model_view_mat", camera.get_view());
		//pipeline.set_uniform(PROGRAM_TYPE::VERTEX, "model_view_proj_mat", camera.get_proj() * camera.get_view());
		//const Mesh& mesh = Triangles("bunny.txt");
		//glEnable(GL_DEPTH_TEST);

		const GLsizei grid_size = 256;
		Voxelize voxelize(grid_size);
		voxelize.bind();
		Triangles bunny_simple("xyzrgb_dragon.obj");
		bunny_simple.bind();
		glEnable(GL_RASTERIZER_DISCARD);
		bunny_simple.draw();
		glDisable(GL_RASTERIZER_DISCARD);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		Raycast raycast(grid_size, voxelize.get_image(), &camera);
		raycast.bind();
		const Mesh& mesh = Quad();
		mesh.bind();

		//LinkedList linkedlist(grid_size, 8);
		//linkedlist.bind();
		//TrianglesAdj bunny_simple("bunny_simple.txt");
		//bunny_simple.bind();
		//glEnable(GL_RASTERIZER_DISCARD);
		//bunny_simple.draw();
		//glDisable(GL_RASTERIZER_DISCARD);
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		//linkedlist.validate();

		//Raycast raycast(grid_size, linkedlist.get_buffer(), &camera);
		//raycast.bind();
		//const Mesh& mesh = Quad();
		//mesh.bind();
		
		//UserContext usercontext = { &camera, &pipeline };
		//glfwSetWindowUserPointer(window, &usercontext);
		while (!glfwWindowShouldClose(window))
		{
			glClearColor(0.f, 153.f/255.f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			mesh.draw();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	catch (const std::string& err)
	{
		std::cerr << err << std::endl;
	}

	return 0;
}