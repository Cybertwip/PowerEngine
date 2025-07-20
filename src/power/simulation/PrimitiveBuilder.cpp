#include "simulation/PrimitiveBuilder.hpp"

#include "actors/Actor.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/PrimitiveComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "filesystem/ImageUtils.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
#include <vector>

// Helper function to create a new material
static std::shared_ptr<MaterialProperties> create_material(const glm::vec4& color) {
	auto material = std::make_shared<MaterialProperties>();
	material->mDiffuse = color;
	return material;
}


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
	material->mDiffuse = glm::vec4(1.0f, 0.5f, 1.0f, 1.0f); // Example color
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

// Helper function to add vertices and indices for a unit cube, transformed and colored.
static void add_cube(std::vector<std::unique_ptr<MeshVertex>>& vertices,
					 std::vector<unsigned int>& indices,
					 uint32_t materialId,
					 const glm::mat4& transform) {
	uint32_t base_vertex = vertices.size();
	
	// 8 vertices of a cube
	glm::vec3 v[8] = {
		{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
		{-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f}
	};
	
	for (int i = 0; i < 8; ++i) {
		auto vertex = std::make_unique<MeshVertex>(glm::vec3(transform * glm::vec4(v[i], 1.0f)));
		vertex->set_material_id(materialId);
		vertices.emplace_back(std::move(vertex));
	}
	
	// 12 triangles for a cube
	unsigned int ind[] = {
		0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
		0, 3, 7, 7, 4, 0, 1, 5, 6, 6, 2, 1,
		3, 2, 6, 6, 7, 3, 0, 1, 5, 5, 4, 0
	};
	
	for (int i = 0; i < 36; ++i) {
		indices.emplace_back(base_vertex + ind[i]);
	}
}

// Helper function to add vertices and indices for a cone, transformed and colored.
static void add_cone(std::vector<std::unique_ptr<MeshVertex>>& vertices,
					 std::vector<unsigned int>& indices,
					 uint32_t materialId,
					 const glm::mat4& transform,
					 int segments = 12) {
	uint32_t base_vertex = vertices.size();
	uint32_t tip_vertex = base_vertex;
	uint32_t base_center_vertex = base_vertex + 1;
	
	// Tip and base center vertices
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(transform * glm::vec4(0, 0.5f, 0, 1.0f))));
	vertices.back()->set_material_id(materialId);
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(transform * glm::vec4(0, -0.5f, 0, 1.0f))));
	vertices.back()->set_material_id(materialId);
	
	// Base vertices
	for (int i = 0; i < segments; ++i) {
		float angle = (float)i / segments * 2.0f * glm::pi<float>();
		float x = cos(angle) * 0.5f;
		float z = sin(angle) * 0.5f;
		vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(transform * glm::vec4(x, -0.5f, z, 1.0f))));
		vertices.back()->set_material_id(materialId);
	}
	
	// Indices for sides and base
	for (int i = 0; i < segments; ++i) {
		indices.emplace_back(tip_vertex);
		indices.emplace_back(base_vertex + 2 + i);
		indices.emplace_back(base_vertex + 2 + (i + 1) % segments);
		
		indices.emplace_back(base_center_vertex);
		indices.emplace_back(base_vertex + 2 + (i + 1) % segments);
		indices.emplace_back(base_vertex + 2 + i);
	}
}

// Generates MeshData for the Translation Gizmo
std::unique_ptr<MeshData> create_translation_gizmo_mesh_data() {
	auto meshData = std::make_unique<MeshData>();
	auto& vertices = meshData->get_vertices();
	auto& indices = meshData->get_indices();
	
	const float shaftLength = 1.5f;
	const float shaftRadius = 0.03f;
	const float headLength = 0.4f;
	const float headRadius = 0.1f;
	
	// Materials: 0=Red(X), 1=Green(Y), 2=Blue(Z)
	meshData->get_material_properties().push_back(create_material({1, 0, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 1, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 0, 1, 1}));
	
	// X-Axis Arrow
	glm::mat4 shaftX = glm::translate(glm::mat4(1.0f), {shaftLength * 0.5f, 0, 0}) * glm::scale(glm::mat4(1.0f), {shaftLength, shaftRadius, shaftRadius});
	add_cube(vertices, indices, 0, shaftX);
	glm::mat4 headX = glm::translate(glm::mat4(1.0f), {shaftLength, 0, 0}) * glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), {0, 0, 1}) * glm::scale(glm::mat4(1.0f), {headLength, headRadius * 2, headRadius * 2});
	add_cone(vertices, indices, 0, headX);
	
	// Y-Axis Arrow
	glm::mat4 shaftY = glm::translate(glm::mat4(1.0f), {0, shaftLength * 0.5f, 0}) * glm::scale(glm::mat4(1.0f), {shaftRadius, shaftLength, shaftRadius});
	add_cube(vertices, indices, 1, shaftY);
	glm::mat4 headY = glm::translate(glm::mat4(1.0f), {0, shaftLength, 0}) * glm::scale(glm::mat4(1.0f), {headRadius * 2, headLength, headRadius * 2});
	add_cone(vertices, indices, 1, headY);
	
	// Z-Axis Arrow
	glm::mat4 shaftZ = glm::translate(glm::mat4(1.0f), {0, 0, shaftLength * 0.5f}) * glm::scale(glm::mat4(1.0f), {shaftRadius, shaftRadius, shaftLength});
	add_cube(vertices, indices, 2, shaftZ);
	glm::mat4 headZ = glm::translate(glm::mat4(1.0f), {0, 0, shaftLength}) * glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), {1, 0, 0}) * glm::scale(glm::mat4(1.0f), {headRadius * 2, headLength, headRadius * 2});
	add_cone(vertices, indices, 2, headZ);
	
	return meshData;
}

std::unique_ptr<MeshData> create_rotation_gizmo_mesh_data() {
	auto meshData = std::make_unique<MeshData>();
	auto& vertices = meshData->get_vertices();
	auto& indices = meshData->get_indices();
	
	const float ringRadius = 1.0f;
	const float tubeRadius = 0.04f;
	const int ringSegments = 48;
	const int tubeSegments = 12;
	
	// Materials: 0=Red(X), 1=Green(Y), 2=Blue(Z)
	meshData->get_material_properties().push_back(create_material({1, 0, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 1, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 0, 1, 1}));
	
	auto create_torus = [&](uint32_t materialId, const glm::quat& rotation) {
		uint32_t base_vertex = vertices.size();
		// Generate vertices
		for (int i = 0; i <= ringSegments; i++) { // Main ring segments
			float u = (float)i / ringSegments * 2.0f * glm::pi<float>();
			float cos_u = cos(u);
			float sin_u = sin(u);
			
			for (int j = 0; j <= tubeSegments; j++) { // Tube segments
				float v = (float)j / tubeSegments * 2.0f * glm::pi<float>();
				float cos_v = cos(v);
				float sin_v = sin(v);
				
				// Parametric equation for a torus centered at the origin
				glm::vec3 pos;
				pos.x = (ringRadius + tubeRadius * cos_v) * cos_u;
				pos.y = (ringRadius + tubeRadius * cos_v) * sin_u;
				pos.z = tubeRadius * sin_v;
				
				// Apply the overall rotation to orient the torus in space
				pos = rotation * pos;
				
				auto vertex = std::make_unique<MeshVertex>(pos);
				vertex->set_material_id(materialId);
				vertices.emplace_back(std::move(vertex));
			}
		}
		
		// Generate indices for the torus surface
		for (int i = 0; i < ringSegments; i++) {
			for (int j = 0; j < tubeSegments; j++) {
				int p1 = base_vertex + i * (tubeSegments + 1) + j;
				int p2 = base_vertex + i * (tubeSegments + 1) + (j + 1);
				int p3 = base_vertex + (i + 1) * (tubeSegments + 1) + (j + 1);
				int p4 = base_vertex + (i + 1) * (tubeSegments + 1) + j;
				indices.insert(indices.end(), { (unsigned)p1, (unsigned)p2, (unsigned)p3 });
				indices.insert(indices.end(), { (unsigned)p1, (unsigned)p3, (unsigned)p4 });
			}
		}
	};
	
	// X-Axis Ring (Red): Circles the X-axis, lies in the YZ plane.
	// Achieved by rotating the default XY-plane torus 90 degrees around the Y-axis.
	create_torus(0, glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)));
	
	// Y-Axis Ring (Green): Circles the Y-axis, lies in the XZ plane.
	// Achieved by rotating the default XY-plane torus 90 degrees around the X-axis.
	create_torus(1, glm::angleAxis(glm::half_pi<float>(), glm::vec3(1, 0, 0)));
	
	// Z-Axis Ring (Blue): Circles the Z-axis, lies in the XY plane.
	// No rotation needed for the default torus.
	create_torus(2, glm::quat(1, 0, 0, 0));
	
	return meshData;
}

// Generates MeshData for the Scale Gizmo
std::unique_ptr<MeshData> create_scale_gizmo_mesh_data() {
	auto meshData = std::make_unique<MeshData>();
	auto& vertices = meshData->get_vertices();
	auto& indices = meshData->get_indices();
	
	const float shaftLength = 1.5f;
	const float shaftRadius = 0.03f;
	const float headSize = 0.15f;
	
	// Materials: 0=Red(X), 1=Green(Y), 2=Blue(Z)
	meshData->get_material_properties().push_back(create_material({1, 0, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 1, 0, 1}));
	meshData->get_material_properties().push_back(create_material({0, 0, 1, 1}));
	
	// X-Axis Handle
	glm::mat4 shaftX = glm::translate(glm::mat4(1.0f), {shaftLength * 0.5f, 0, 0}) * glm::scale(glm::mat4(1.0f), {shaftLength, shaftRadius, shaftRadius});
	add_cube(vertices, indices, 0, shaftX);
	glm::mat4 headX = glm::translate(glm::mat4(1.0f), {shaftLength, 0, 0}) * glm::scale(glm::mat4(1.0f), glm::vec3(headSize));
	add_cube(vertices, indices, 0, headX);
	
	// Y-Axis Handle
	glm::mat4 shaftY = glm::translate(glm::mat4(1.0f), {0, shaftLength * 0.5f, 0}) * glm::scale(glm::mat4(1.0f), {shaftRadius, shaftLength, shaftRadius});
	add_cube(vertices, indices, 1, shaftY);
	glm::mat4 headY = glm::translate(glm::mat4(1.0f), {0, shaftLength, 0}) * glm::scale(glm::mat4(1.0f), glm::vec3(headSize));
	add_cube(vertices, indices, 1, headY);
	
	// Z-Axis Handle
	glm::mat4 shaftZ = glm::translate(glm::mat4(1.0f), {0, 0, shaftLength * 0.5f}) * glm::scale(glm::mat4(1.0f), {shaftRadius, shaftRadius, shaftLength});
	add_cube(vertices, indices, 2, shaftZ);
	glm::mat4 headZ = glm::translate(glm::mat4(1.0f), {0, 0, shaftLength}) * glm::scale(glm::mat4(1.0f), glm::vec3(headSize));
	add_cube(vertices, indices, 2, headZ);
	
	return meshData;
}


std::unique_ptr<MeshData> PrimitiveBuilder::create_mesh_data(PrimitiveShape primitiveShape) {
	switch (primitiveShape) {
		case PrimitiveShape::TranslationGizmo:
			return create_translation_gizmo_mesh_data();
		case PrimitiveShape::RotationGizmo:
			return create_rotation_gizmo_mesh_data();
		case PrimitiveShape::ScaleGizmo:
			return create_scale_gizmo_mesh_data();
		case PrimitiveShape::Cube:
			return create_cube_mesh_data();
		case PrimitiveShape::Sphere:
			return create_sphere_mesh_data();
		case PrimitiveShape::Cuboid:
			// Example dimensions scaled by SCALE_FACTOR; these could be parameters if needed
			return create_cuboid_mesh_data(1.0f, 2.0f, 1.0f);
		default:
			throw std::invalid_argument("Unsupported or unimplemented PrimitiveShape for gizmo.");
	}
}

PrimitiveBuilder::PrimitiveBuilder(IMeshBatch& meshBatch)
: mMeshBatch(meshBatch) {}

Actor& PrimitiveBuilder::build(Actor& actor, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	std::unique_ptr<MeshData> meshData = create_mesh_data(primitiveShape);
	if (!meshData) {
		throw std::runtime_error("Failed to create MeshData for the specified PrimitiveShape.");
	}
	
	auto& metadataComponent = actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	
	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(
														*meshData,
														meshShader,
														mMeshBatch,
														metadataComponent,
														colorComponent
														);
	
	auto primitiveComponent = std::make_unique<PrimitiveComponent>(std::move(mesh), std::move(meshData));
	actor.add_component<DrawableComponent>(std::make_unique<DrawableComponent>(std::move(primitiveComponent)));
	actor.add_component<TransformComponent>();
	
	return actor;
}

// Helper function to create MeshData for a Sprite.
// This creates a two-sided, textured quad.
std::unique_ptr<MeshData> create_sprite_mesh_data(float width, float height, const std::string& texturePath) {
	auto meshData = std::make_unique<MeshData>();
	
	// Calculate scaled half-dimensions for the vertices
	const float half_width_scaled = (width / 2.0f) * SCALE_FACTOR / 10.0f;
	const float half_height_scaled = (height / 2.0f) * SCALE_FACTOR / 10.0f;
	
	// Define 4 vertices for a quad in the XY plane using the scaled dimensions
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-half_width_scaled, -half_height_scaled, 0.0f))); // 0: Bottom-Left
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( half_width_scaled, -half_height_scaled, 0.0f))); // 1: Bottom-Right
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( half_width_scaled,  half_height_scaled, 0.0f))); // 2: Top-Right
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-half_width_scaled,  half_height_scaled, 0.0f))); // 3: Top-Left
	
	// Define the normal vector for the sprite (facing the positive Z-axis)
	glm::vec3 normal(0.0f, 0.0f, 1.0f);
	
	// Define texture coordinates for each vertex (V-coordinate is flipped)
	glm::vec2 tex_coords[] = {
		{0.0f, 1.0f}, // For vertex 0 (Bottom-Left) -> Maps to Top-Left of texture
		{1.0f, 1.0f}, // For vertex 1 (Bottom-Right) -> Maps to Top-Right of texture
		{1.0f, 0.0f}, // For vertex 2 (Top-Right) -> Maps to Bottom-Right of texture
		{0.0f, 0.0f}  // For vertex 3 (Top-Left) -> Maps to Bottom-Left of texture
	};
	
	// Assign properties (normal, texture coordinates, material ID) to each vertex
	for (size_t i = 0; i < vertices.size(); ++i) {
		vertices[i]->set_normal(normal);
		vertices[i]->set_texture_coords1(tex_coords[i]);
		vertices[i]->set_material_id(0);
	}
	
	meshData->get_vertices() = std::move(vertices);
	
	// Define indices for a two-sided quad.
	// The renderer/shader should have back-face culling disabled for this material.
	std::vector<unsigned int> indices = {
		// Front face (counter-clockwise winding)
		0, 1, 2,
		2, 3, 0,
		// Back face (clockwise winding, appears counter-clockwise from the back)
		0, 2, 1,
		2, 0, 3
	};
	meshData->get_indices() = std::move(indices);
	
	// Assign material properties
	auto material = create_material({1.0f, 1.0f, 1.0f, 1.0f}); // Default white diffuse color
	
	// If a valid path was provided, try to load the texture file.
	if (!texturePath.empty() && std::filesystem::exists(texturePath)) {
		std::cout << "Found texture file, loading: " << texturePath << std::endl;
		std::ifstream file(texturePath, std::ios::binary);
		if (file) {
			// Read file into a buffer
			std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
			// Create texture from the file data in memory.
			material->mTextureDiffuse =  std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
			
			material->mHasDiffuseTexture = true;
		}
	}
	
	// A hint for the renderer to disable back-face culling could be set here
	// material->mTwoSided = true;
	meshData->get_material_properties().push_back(material);
	
	return meshData;
}
// This is the new overload for building sprites.
Actor& PrimitiveBuilder::build_sprite(Actor& actor, const std::string& actorName, const std::string& texturePath, float width, float height, ShaderWrapper& meshShader) {
	std::unique_ptr<MeshData> meshData = create_sprite_mesh_data(width, height, texturePath);
	if (!meshData) {
		throw std::runtime_error("Failed to create MeshData for the sprite.");
	}
	
	auto& metadataComponent = actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	
	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(
														*meshData,
														meshShader,
														mMeshBatch,
														metadataComponent,
														colorComponent
														);
	
	auto primitiveComponent = std::make_unique<PrimitiveComponent>(std::move(mesh), std::move(meshData));
	actor.add_component<DrawableComponent>(std::make_unique<DrawableComponent>(std::move(primitiveComponent)));
	actor.add_component<TransformComponent>();
	
	return actor;
}
