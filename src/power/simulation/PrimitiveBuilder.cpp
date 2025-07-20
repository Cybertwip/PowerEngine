#include "simulation/PrimitiveBuilder.hpp"

#include "actors/Actor.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/PrimitiveComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
#include <vector>

// Helper to create a material. Assumed to exist from your previous code.
static std::shared_ptr<MaterialProperties> create_material(const glm::vec4& color) {
	auto material = std::make_shared<MaterialProperties>();
	material->mDiffuse = color;
	return material;
}

// Helper function to create MeshData for a Sprite.
// This creates a two-sided, textured quad.
std::unique_ptr<MeshData> create_sprite_mesh_data(float width, float height, const std::string& texturePath) {
	auto meshData = std::make_unique<MeshData>();
	
	// Define 4 vertices for a quad in the XY plane
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2.0f, -height / 2.0f, 0.0f))); // 0: Bottom-Left
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2.0f, -height / 2.0f, 0.0f))); // 1: Bottom-Right
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3( width / 2.0f,  height / 2.0f, 0.0f))); // 2: Top-Right
	vertices.emplace_back(std::make_unique<MeshVertex>(glm::vec3(-width / 2.0f,  height / 2.0f, 0.0f))); // 3: Top-Left
	
	glm::vec3 normal(0.0f, 0.0f, 1.0f);
	glm::vec2 tex_coords[] = {
		{0.0f, 0.0f}, // For vertex 0
		{1.0f, 0.0f}, // For vertex 1
		{1.0f, 1.0f}, // For vertex 2
		{0.0f, 1.0f}  // For vertex 3
	};
	
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
	
	// If a valid path was found, try to load the file.
	if (!texturePath.empty() && std::filesystem::exists(texturePath)) {
		std::cout << "Found texture file, loading: " << texturePath << std::endl;
		std::ifstream file(texturePath, std::ios::binary);
		if (file) {
			std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
			// Create texture from the file data in memory.
			// This assumes nanogui::Texture can be constructed from a memory buffer of a PNG/JPG.
			material->mTextureDiffuse =  std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
			
			material->mHasDiffuseTexture = true;

		}
	}

//	material->mTwoSided = true; // A hint for the renderer to disable back-face culling
	meshData->get_material_properties().push_back(material);
	
	return meshData;
}


PrimitiveBuilder::PrimitiveBuilder(IMeshBatch& meshBatch)
: mMeshBatch(meshBatch) {}

// This is the original build function for enum-based shapes.
// NOTE: It should no longer handle PrimitiveShape::Sprite.
Actor& PrimitiveBuilder::build(Actor& actor, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	// This function calls a dispatcher like `create_mesh_data(primitiveShape)`
	// which is assumed to exist from your original file. That dispatcher should
	// not have a case for `Sprite` anymore.
	// ... (rest of the original implementation)
	return actor;
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
