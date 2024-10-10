#include "simulation/PrimitiveBuilder.hpp"

#include "actors/Actor.hpp"

#include "components/ColorComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/PrimitiveComponent.hpp"

#include "graphics/drawing/Mesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <stdexcept>
#include <vector>
#include <cmath>

// Scaling factor
const float SCALE_FACTOR = 100.0f;

// Helper function to create MeshData for a Cube
std::unique_ptr<MeshData> create_cube_mesh_data() {
	auto meshData = std::make_unique<MeshData>();
	
	// Define cube vertices scaled by SCALE_FACTOR
	// 8 vertices of a cube
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( 0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( 0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( 0.5f * SCALE_FACTOR, -0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( 0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR,  0.5f * SCALE_FACTOR)));
	
	// Assign normals and texture coordinates as needed
	// For simplicity, setting default normals and tex coords
	for (auto& vertex : vertices) {
		vertex->set_normal(glm::vec3(0.0f, 0.0f, 1.0f));
		vertex->set_texture_coords1(glm::vec2(0.0f, 0.0f));
		vertex->set_texture_coords2(glm::vec2(0.0f, 0.0f));
		vertex->set_material_id(0); // Default material ID
		vertex->set_color(glm::vec4(1.0f)); // White color
	}
	
	meshData->get_vertices() = std::move(vertices);
	
	// Define cube indices (12 triangles)
	std::vector<unsigned int> indices = {
		// Front face
		0, 1, 2,
		2, 3, 0,
		// Back face
		4, 6, 5,
		6, 4, 7,
		// Left face
		4, 0, 3,
		3, 7, 4,
		// Right face
		1, 5, 6,
		6, 2, 1,
		// Top face
		3, 2, 6,
		6, 7, 3,
		// Bottom face
		4, 5, 1,
		1, 0, 4
	};
	
	meshData->get_indices() = std::move(indices);
	
	// Assign material properties
	auto material = std::make_shared<MaterialProperties>();
	material->mDiffuse = glm::vec4(1.0f, 0.5f, 0.31f, 1.0f); // Example color
	meshData->get_material_properties().push_back(material);
	
	return meshData;
}

// Helper function to create MeshData for a Sphere
std::unique_ptr<MeshData> create_sphere_mesh_data() {
	auto meshData = std::make_unique<MeshData>();
	
	const int sectorCount = 36;
	const int stackCount = 18;
	const float radius = 0.5f * SCALE_FACTOR; // Scaled radius
	
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	std::vector<unsigned int> indices;
	
	float x, y, z, xy;                              // vertex position
	float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
	float s, t;                                     // vertex tex coord
	
	float sectorStep = 2 * M_PI / sectorCount;
	float stackStep = M_PI / stackCount;
	float sectorAngle, stackAngle;
	
	// Generate vertices
	for(int i = 0; i <= stackCount; ++i)
	{
		stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)
		
		// add (sectorCount+1) vertices per stack
		for(int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi
			
			// vertex position scaled by SCALE_FACTOR
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			// normalized vertex normal
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			// vertex tex coord
			s = (float)j / sectorCount;
			t = (float)i / stackCount;
			
			auto vertex = std::make_unique<MeshVertex>(glm::vec3(x, y, z));
			vertex->set_normal(glm::vec3(nx, ny, nz));
			vertex->set_texture_coords1(glm::vec2(s, t));
			vertex->set_texture_coords2(glm::vec2(0.0f, 0.0f));
			vertex->set_material_id(0); // Default material ID
			vertex->set_color(glm::vec4(1.0f)); // White color
			vertices.emplace_back(std::move(vertex));
		}
	}
	
	meshData->get_vertices() = std::move(vertices);
	
	// Generate indices
	for(int i = 0; i < stackCount; ++i)
	{
		int k1 = i * (sectorCount + 1);     // beginning of current stack
		int k2 = k1 + sectorCount + 1;      // beginning of next stack
		
		for(int j = 0; j < sectorCount; ++j, ++k1, ++k2)
		{
			if(i != 0)
			{
				indices.emplace_back(k1);
				indices.emplace_back(k2);
				indices.emplace_back(k1 + 1);
			}
			
			if(i != (stackCount-1))
			{
				indices.emplace_back(k1 + 1);
				indices.emplace_back(k2);
				indices.emplace_back(k2 + 1);
			}
		}
	}
	
	meshData->get_indices() = std::move(indices);
	
	// Assign material properties
	auto material = std::make_shared<MaterialProperties>();
	material->mDiffuse = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f); // Example color
	meshData->get_material_properties().push_back(material);
	
	return meshData;
}

// Helper function to create MeshData for a Cuboid
std::unique_ptr<MeshData> create_cuboid_mesh_data(float width, float height, float depth) {
	auto meshData = std::make_unique<MeshData>();
	
	// Define cuboid vertices scaled by SCALE_FACTOR
	// 8 vertices of a cuboid
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2 * SCALE_FACTOR, -height / 2 * SCALE_FACTOR, -depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2 * SCALE_FACTOR, -height / 2 * SCALE_FACTOR, -depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2 * SCALE_FACTOR,  height / 2 * SCALE_FACTOR, -depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2 * SCALE_FACTOR,  height / 2 * SCALE_FACTOR, -depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2 * SCALE_FACTOR, -height / 2 * SCALE_FACTOR,  depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2 * SCALE_FACTOR, -height / 2 * SCALE_FACTOR,  depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2 * SCALE_FACTOR,  height / 2 * SCALE_FACTOR,  depth / 2 * SCALE_FACTOR)));
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2 * SCALE_FACTOR,  height / 2 * SCALE_FACTOR,  depth / 2 * SCALE_FACTOR)));
	
	// Assign normals and texture coordinates as needed
	// For simplicity, setting default normals and tex coords
	for (auto& vertex : vertices) {
		vertex->set_normal(glm::vec3(0.0f, 0.0f, 1.0f));
		vertex->set_texture_coords1(glm::vec2(0.0f, 0.0f));
		vertex->set_texture_coords2(glm::vec2(0.0f, 0.0f));
		vertex->set_material_id(0); // Default material ID
		vertex->set_color(glm::vec4(1.0f)); // White color
	}
	
	meshData->get_vertices() = std::move(vertices);
	
	// Define cuboid indices (12 triangles)
	std::vector<unsigned int> indices = {
		// Front face
		0, 1, 2,
		2, 3, 0,
		// Back face
		4, 6, 5,
		6, 4, 7,
		// Left face
		4, 0, 3,
		3, 7, 4,
		// Right face
		1, 5, 6,
		6, 2, 1,
		// Top face
		3, 2, 6,
		6, 7, 3,
		// Bottom face
		4, 5, 1,
		1, 0, 4
	};
	
	meshData->get_indices() = std::move(indices);
	
	// Assign material properties
	auto material = std::make_shared<MaterialProperties>();
	material->mDiffuse = glm::vec4(0.5f, 1.0f, 0.5f, 1.0f); // Example color
	meshData->get_material_properties().push_back(material);
	
	return meshData;
}

std::unique_ptr<MeshData> PrimitiveBuilder::create_mesh_data(PrimitiveShape primitiveShape) {
	switch (primitiveShape) {
		case PrimitiveShape::Cube:
			return create_cube_mesh_data();
		case PrimitiveShape::Sphere:
			return create_sphere_mesh_data();
		case PrimitiveShape::Cuboid:
			// Example dimensions scaled by SCALE_FACTOR; these could be parameters if needed
			return create_cuboid_mesh_data(1.0f, 2.0f, 1.0f);
		default:
			throw std::invalid_argument("Unsupported PrimitiveShape.");
	}
}

PrimitiveBuilder::PrimitiveBuilder(IMeshBatch& meshBatch)
: mMeshBatch(meshBatch) {
}

Actor& PrimitiveBuilder::build(Actor& actor, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	// Create MeshData for the specified primitive shape
	std::unique_ptr<MeshData> meshData = create_mesh_data(primitiveShape);
	if (!meshData) {
		throw std::runtime_error("Failed to create MeshData for the specified PrimitiveShape.");
	}
	
	auto& metadataComponent = actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	
	// Retrieve or create ColorComponent
	// Assuming the Actor does not already have a ColorComponent
	auto& colorComponent = actor.add_component<ColorComponent>(metadataComponent);
	// Optionally, set color based on the MeshData or other criteria
	
	// Create Mesh
	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(
														std::move(meshData),
														meshShader,
														mMeshBatch,
														colorComponent
														);
	
	// Create PrimitiveComponent with the Mesh
	std::unique_ptr<PrimitiveComponent> primitiveComponent = std::make_unique<PrimitiveComponent>(std::move(mesh));
	
	// Create DrawableComponent and add to Actor
	// If PrimitiveComponent is a Drawable, it can be wrapped in a DrawableComponent
	std::unique_ptr<DrawableComponent> drawableComponent = std::make_unique<DrawableComponent>(std::move(primitiveComponent));
	
	actor.add_component<DrawableComponent>(std::move(drawableComponent));
	
	// Add TransformComponent for positioning and scaling
	auto& transformComponent = actor.add_component<TransformComponent>();
	transformComponent.set_scale(glm::vec3(SCALE_FACTOR)); // Ensure the scale is consistent
	
	return actor;
}
