#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <GLFW/glfw3.h>

#include <cmath>

Mesh::Vertex::Vertex(){
	
}

Mesh::Vertex::Vertex(const glm::vec3 &pos, const glm::vec2 &tex)
: mPosition(pos), mNormal(0.0f), mTexCoords1(tex), mTexCoords2(tex)
{
	
}

void Mesh::Vertex::set_position(const glm::vec3 &vec)
{
	mPosition = vec;
}

void Mesh::Vertex::set_normal(const glm::vec3 &vec)
{
	mNormal = vec;
}

void Mesh::Vertex::set_texture_coords1(const glm::vec2 &vec)
{
	mTexCoords1 = vec;
}

void Mesh::Vertex::set_texture_coords2(const glm::vec2 &vec)
{
	mTexCoords2 = vec;
}

glm::vec3 Mesh::Vertex::get_position() const {
	return mPosition;
}

glm::vec3 Mesh::Vertex::get_normal() const {
	return mNormal;
}

glm::vec2 Mesh::Vertex::get_tex_coords1() const {
	return mTexCoords1;
}

glm::vec2 Mesh::Vertex::get_tex_coords2() const {
	return mTexCoords2;
}

Mesh::MeshShader::MeshShader(nanogui::Shader& shader) : ShaderWrapper(shader) {
	
}

void Mesh::MeshShader::upload_vertex_data(const std::vector<Vertex>& vertexData){
	std::vector<float> positions;
	std::vector<float> normals;
	std::vector<float> texCoords1;
	std::vector<float> texCoords2;
//	std::vector<float> tangents;
//	std::vector<float> bitangents;
	std::vector<int> boneIds;
	std::vector<float> weights;
	for (const auto& vertex : vertexData) {
		positions.insert(positions.end(), {vertex.get_position().x, vertex.get_position().y, vertex.get_position().z});
		normals.insert(normals.end(), {vertex.get_normal().x, vertex.get_normal().y, vertex.get_normal().z});
		texCoords1.insert(texCoords1.end(), {vertex.get_tex_coords1().x, vertex.get_tex_coords1().y});
		texCoords2.insert(texCoords2.end(), {vertex.get_tex_coords2().x, vertex.get_tex_coords2().y});
	}
	
	mShader.set_buffer("position", nanogui::VariableType::Float32, {vertexData.size(), 3}, positions.data());
	//mShader.set_buffer("normal", nanogui::VariableType::Float32, {vertexData.size(), 3}, normals.data());
	//mShader.set_buffer("texCoords1", nanogui::VariableType::Float32, {vertexData.size(), 2}, texCoords1.data());
	//mShader.set_buffer("texCoords2", nanogui::VariableType::Float32, {vertexData.size(), 2}, texCoords2.data());
	//mShader.set_buffer("tangent", nanogui::VariableType::Float32, {vertexData.size(), 3}, tangents.data());
	//mShader.set_buffer("bitangent", nanogui::VariableType::Float32, {vertexData.size(), 3}, bitangents.data());
	//mShader.set_buffer("boneIds", nanogui::VariableType::Int32, {vertexData.size(), MAX_BONE_INFLUENCE}, boneIds.data());
	//mShader.set_buffer("weights", nanogui::VariableType::Float32, {vertexData.size(), MAX_BONE_INFLUENCE}, weights.data());
	
}


Mesh::Mesh(MeshShader& shader)
: mShader(shader) {
	initialize_mesh();
}

void Mesh::initialize_mesh() {
	
	std::vector<uint32_t> indices = {
		3, 2, 6, 6, 7, 3,
		4, 5, 1, 1, 0, 4,
		4, 0, 3, 3, 7, 4,
		1, 5, 6, 6, 2, 1,
		0, 1, 2, 2, 3, 0,
		7, 6, 5, 5, 4, 7
	};

	
	std::vector<glm::vec3> positions = {
		glm::vec3(-1.f, 1.f, 1.f), glm::vec3(-1.f, -1.f, 1.f),
		glm::vec3(1.f, -1.f, 1.f), glm::vec3(1.f, 1.f, 1.f),
		glm::vec3(-1.f, 1.f, -1.f), glm::vec3(-1.f, -1.f, -1.f),
		glm::vec3(1.f, -1.f, -1.f), glm::vec3(1.f, 1.f, -1.f)
	};
	
	std::vector<glm::vec3> colors = {
		glm::vec3(0.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 1.f),
		glm::vec3(1.f, 0.f, 1.f), glm::vec3(1.f, 1.f, 1.f),
		glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 0.f),
		glm::vec3(1.f, 0.f, 0.f), glm::vec3(1.f, 1.f, 0.f)
	};
	
	std::vector<Vertex> vertices;
	for (size_t i = 0; i < positions.size(); ++i)
	{
		vertices.emplace_back(positions[i], colors[i]);
	}
	
	mShader.upload_index_data(indices);
	mShader.upload_vertex_data(vertices);
}

void Mesh::draw_content(Canvas& canvas) {
	using namespace nanogui;
	
	Matrix4f view = Matrix4f::look_at(
									  Vector3f(0, -2, -10),
									  Vector3f(0, 0, 0),
									  Vector3f(0, 1, 0)
									  );
	
	Matrix4f model = Matrix4f::rotate(
									  Vector3f(0, 1, 0),
									  (float) glfwGetTime()
									  );
	
	Matrix4f proj = Matrix4f::perspective(
										  float(25 * M_PI / 180),
										  0.1f,
										  20.f,
										  1.0f // Aspect ratio will be set in the Canvas
										  );
	
	mvp = proj * view * model;
	
	mShader.set_uniform("mvp", mvp);
	
	mShader.begin();
	mShader.draw_array(Shader::PrimitiveType::Triangle, 0, 12*3, true);
	mShader.end();
}