#pragma once

enum VERTEX_ATTRIB { POSITION, NORMAL, INDEX, NUM_ATTRIB };
class Mesh
{
public:
	Mesh() 
	{ // constructor body of base class is executed first
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao); // buffer data format descriptor
	}
	virtual ~Mesh() { glDeleteVertexArrays(1, &vao); }

	void bind() const { glBindVertexArray(vao); }
	virtual void draw() const = 0;
protected:
	GLuint vao;
};

class Quad : public Mesh
{
public:
	Quad();
	~Quad() { glDeleteBuffers(1, &bo); }

	void draw() const
	{
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2 triangles
	}
private:
	GLuint bo;
};

typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;
class Triangles : public Mesh
{
public:
	explicit Triangles(const char* file_name);
	~Triangles() { glDeleteBuffers(VERTEX_ATTRIB::NUM_ATTRIB, bo); }

	void draw() const { glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0); }
protected:
	Triangles() {};
	void load_file(const char* file_name, TriMesh& mesh);
	void load_buffer(const TriMesh& mesh, const std::vector<GLuint>& indices);

	GLsizei count;
	GLuint bo[VERTEX_ATTRIB::NUM_ATTRIB];
};

class TrianglesAdj : public Triangles
{
public:
	explicit TrianglesAdj(const char* file_name);
	~TrianglesAdj() {}

	void draw() const { glDrawElements(GL_TRIANGLES_ADJACENCY, count, GL_UNSIGNED_INT, 0); }
};

class Triangle : public Mesh
{
public:
	Triangle();
	~Triangle() { glDeleteBuffers(1, &bo); }

	void draw() const
	{
		glDrawArrays(GL_TRIANGLES, 0, 6); // two triangle
	}
private:
	GLuint bo;
};