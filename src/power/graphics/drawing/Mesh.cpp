#include "Mesh.hpp"

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "components/ColorComponent.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "import/Fbx.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/renderpass.h>
#include <nanogui/texture.h>

#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
#include <nanogui/opengl.h>
#elif defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif

#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <algorithm>

Mesh::Mesh(std::unique_ptr<MeshData> meshData, ShaderWrapper& shader, MeshBatch& meshBatch, ColorComponent& colorComponent)
: mMeshData(std::move(meshData)), mShader(shader), mMeshBatch(meshBatch), mColorComponent(colorComponent), mModelMatrix(nanogui::Matrix4f::identity()) {
	
	if (!mMeshData) {
		throw std::invalid_argument("MeshData cannot be null.");
	}
	
	size_t numVertices = mMeshData->get_vertices().size();
	
	// Pre-allocate flattened data vectors
	mFlattenedPositions.resize(numVertices * 3);
	mFlattenedNormals.resize(numVertices * 3);
	mFlattenedTexCoords1.resize(numVertices * 2);
	mFlattenedTexCoords2.resize(numVertices * 2);
	mFlattenedColors.resize(numVertices * 4); // vec4 per vertex
	
	// Decide on the number of texture IDs per vertex (assuming 1 for simplicity)
	mFlattenedMaterialIds.resize(numVertices); // Single texture ID per vertex
	
	// Flatten the vertex data
	for (size_t i = 0; i < numVertices; ++i) {
		const auto& vertexPtr = mMeshData->get_vertices()[i];
		if (!vertexPtr) {
			throw std::runtime_error("Vertex pointer is null.");
		}
		const auto& vertex = *vertexPtr;
		
		// Positions
		mFlattenedPositions[i * 3 + 0] = vertex.get_position().x;
		mFlattenedPositions[i * 3 + 1] = vertex.get_position().y;
		mFlattenedPositions[i * 3 + 2] = vertex.get_position().z;
		
		// Normals
		mFlattenedNormals[i * 3 + 0] = vertex.get_normal().x;
		mFlattenedNormals[i * 3 + 1] = vertex.get_normal().y;
		mFlattenedNormals[i * 3 + 2] = vertex.get_normal().z;
		
		// Texture Coordinates 1
		mFlattenedTexCoords1[i * 2 + 0] = vertex.get_tex_coords1().x;
		mFlattenedTexCoords1[i * 2 + 1] = vertex.get_tex_coords1().y;
		
		// Texture Coordinates 2
		mFlattenedTexCoords2[i * 2 + 0] = vertex.get_tex_coords2().x;
		mFlattenedTexCoords2[i * 2 + 1] = vertex.get_tex_coords2().y;
		
		// Texture IDs (single ID per vertex)
		mFlattenedMaterialIds[i] = vertex.get_material_id();
		
		// Flattened Colors
		const glm::vec4& color = vertex.get_color();
		mFlattenedColors[i * 4 + 0] = color.r;
		mFlattenedColors[i * 4 + 1] = color.g;
		mFlattenedColors[i * 4 + 2] = color.b;
		mFlattenedColors[i * 4 + 3] = color.a;
	}
	
	mModelMatrix = nanogui::Matrix4f::identity(); // Or any other transformation
	
	// Properly pass a reference_wrapper<Mesh> to add_mesh
	mMeshBatch.add_mesh(std::ref(*this));
	
	// Append the mesh (assuming append takes Mesh&)
	mMeshBatch.append(*this);
}

Mesh::~Mesh() {
	mMeshBatch.remove(*this);
}

void Mesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							   const nanogui::Matrix4f& projection) {
	
	mModelMatrix = model;
}
