#include "stdafx.h"
#include "Mesh.h"

Quad::Quad()
{
	std::array<glm::vec2, 6> positions = {
		glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1),
		glm::vec2(1, 1), glm::vec2(-1, 1), glm::vec2(-1, -1)
	};
	
	glGenBuffers(1, &bo);

	glBindBuffer(GL_ARRAY_BUFFER, bo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VERTEX_ATTRIB::POSITION);
	glVertexAttribPointer(VERTEX_ATTRIB::POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

Triangles::Triangles(const char* file_name)
{
	TriMesh mesh;
	load_file(file_name, mesh);

	std::vector<GLuint> indices(mesh.n_faces() * 3, 0);
	for (auto f : mesh.faces())
	{
		GLuint i = 0;
		for (auto fv : mesh.fv_range(f))
		{
			indices[f.idx() * 3 + i] = fv.idx();
			i++;
		}
	}

	load_buffer(mesh, indices);
}

void Triangles::load_file(const char* file_name, TriMesh& mesh)
{
	std::ifstream is(std::string(file_name), std::ios::in);
	OpenMesh::IO::Options ropt;
	mesh.request_vertex_normals();
	if (!OpenMesh::IO::read_mesh(mesh, is, ".obj", ropt))
	{
		throw "error loading mesh from file " + std::string(file_name);
	}
	printf("Loaded mesh %s\n", file_name);
	std::cout << "# Vertices: " << mesh.n_vertices() << std::endl;
	std::cout << "# Edges   : " << mesh.n_edges() << std::endl;
	std::cout << "# Faces   : " << mesh.n_faces() << std::endl;
	if (!mesh.has_vertex_normals())
	{
		throw "angle weighted vertex normal is required";
	}
}

void Triangles::load_buffer(const TriMesh& mesh, const std::vector<GLuint>& indices)
{
	glGenBuffers(VERTEX_ATTRIB::NUM_ATTRIB, bo);

	glBindBuffer(GL_ARRAY_BUFFER, bo[VERTEX_ATTRIB::POSITION]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.n_vertices() * 3, mesh.points(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VERTEX_ATTRIB::POSITION);
	glVertexAttribPointer(VERTEX_ATTRIB::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, bo[VERTEX_ATTRIB::NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.n_vertices() * 3, mesh.vertex_normals(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VERTEX_ATTRIB::NORMAL);
	glVertexAttribPointer(VERTEX_ATTRIB::NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[VERTEX_ATTRIB::INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
	count = indices.size();
}

TrianglesAdj::TrianglesAdj(const char* file_name)
{
	TriMesh mesh;
	load_file(file_name, mesh);

	std::vector<GLuint> indices(mesh.n_faces() * 6, 0);
	for (auto f : mesh.faces())
	{
		GLuint i = 0;
		for (auto fh : mesh.fh_range(f))
		{
			indices[f.idx() * 6 + i] = mesh.from_vertex_handle(fh).idx(); // 0, 2, 4; 1, 3, 5
			indices[f.idx() * 6 + i + 1] = mesh.to_vertex_handle(mesh.next_halfedge_handle(mesh.opposite_halfedge_handle(fh))).idx();

			i += 2;
		}
	}

	load_buffer(mesh, indices);
}

Triangle::Triangle()
{
	std::array<glm::vec3, 6> positions = {
		glm::vec3(1.f, 0.f, 1.f), glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), 
		glm::vec3(1.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(-1.f, 0.f, 1.f)
	};

	glGenBuffers(1, &bo);

	glBindBuffer(GL_ARRAY_BUFFER, bo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(VERTEX_ATTRIB::POSITION);
	glVertexAttribPointer(VERTEX_ATTRIB::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
}
