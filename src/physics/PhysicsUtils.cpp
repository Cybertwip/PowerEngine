#include "physics/PhysicsUtils.h"
#include "graphics/opengl/gl_mesh.h"

namespace physics{
std::unique_ptr<anim::Mesh> CreateCuboid(const glm::vec3& extents) {
	
	auto vertices = CreateCuboidVertices(extents);
	
	auto mesh = std::make_unique<anim::gl::GLMesh>(vertices);
	mesh->get_mutable_mat_properties().opacity = 0.15f;
	mesh->get_mutable_mat_properties().diffuse = glm::vec3(0.5f, 0.0f, 0.5f);
	
	return mesh;
}

std::vector<anim::Vertex> CreateCuboidVertices(const glm::vec3& extents) {
	const float half_x = extents.x / 2.0f;
	const float half_y = extents.y / 2.0f;
	const float half_z = extents.z / 2.0f;
	
	const float positions[8 * 3] = {
		-half_x, half_y, half_z,     // 0 Front-top-left
		half_x, half_y, half_z,      // 1 Front-top-right
		half_x, half_y, -half_z,     // 2 Back-top-right
		-half_x, half_y, -half_z,    // 3 Back-top-left
		-half_x, -half_y, half_z,    // 4 Front-bottom-left
		half_x, -half_y, half_z,     // 5 Front-bottom-right
		half_x, -half_y, -half_z,    // 6 Back-bottom-right
		-half_x, -half_y, -half_z    // 7 Back-bottom-left
	};

	const unsigned int indices[] = {
		0, 1, 5, 0, 5, 4,   // Front face
		1, 2, 6, 1, 6, 5,   // Right face
		2, 3, 7, 2, 7, 6,   // Back face
		3, 0, 4, 3, 4, 7,   // Left face
		0, 3, 2, 0, 2, 1,   // Top face
		4, 5, 6, 4, 6, 7    // Bottom face
	};
	
	std::vector<anim::Vertex> vertices;
	for (int i = 0; i < 36; ++i) {
		anim::Vertex vertex;
		vertex.set_position(glm::vec3(positions[indices[i] * 3], positions[indices[i] * 3 + 1], positions[indices[i] * 3 + 2]));
		
		glm::vec3 normal(0.0f);
		int face = i / 6;
		switch (face) {
			case 0: normal = glm::vec3(0, 0, 1); break; // Front
			case 1: normal = glm::vec3(1, 0, 0); break; // Right
			case 2: normal = glm::vec3(0, 0, -1); break; // Back
			case 3: normal = glm::vec3(-1, 0, 0); break; // Left
			case 4: normal = glm::vec3(0, -1, 0); break; // Top
			case 5: normal = glm::vec3(0, 1, 0); break; // Bottom
		}
		vertex.set_normal(normal);
		
		vertices.push_back(vertex);
	}
	
	return vertices;
}

}
