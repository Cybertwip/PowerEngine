#include "SkinnedMesh.hpp"

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "components/ColorComponent.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"
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

SkinnedMesh::SkinnedMesh(
						 std::unique_ptr<SkinnedMeshData> skinnedMeshData,
						 ShaderWrapper& shader,
						 SkinnedMeshBatch& meshBatch,
						 ColorComponent& colorComponent,
						 SkinnedAnimationComponent& skinnedComponent)
: mSkinnedMeshData(std::move(skinnedMeshData)),
mShader(shader),
mMeshBatch(meshBatch),
mColorComponent(colorComponent),
mSkinnedComponent(skinnedComponent),
mModelMatrix(nanogui::Matrix4f::identity()) {
	
	size_t numVertices = mSkinnedMeshData->get_skinned_vertices().size();
	
	// Pre-allocate flattened data vectors
	mFlattenedPositions.resize(numVertices * 3);
	mFlattenedNormals.resize(numVertices * 3);
	mFlattenedTexCoords1.resize(numVertices * 2);
	mFlattenedTexCoords2.resize(numVertices * 2);
	mFlattenedColors.resize(numVertices * 4); // vec4 per vertex
	
	mFlattenedBoneIds.resize(numVertices * SkinnedMeshVertex::MAX_BONE_INFLUENCE);
	mFlattenedWeights.resize(numVertices * SkinnedMeshVertex::MAX_BONE_INFLUENCE);
	
	mFlattenedTextureIds.resize(numVertices); // One texture ID per vertex
	
	// Flatten the vertex data
	for (size_t i = 0; i < numVertices; ++i) {
		const auto& vertex = mSkinnedMeshData->get_skinned_vertices()[i];
		
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
		
		// Texture IDs
		mFlattenedTextureIds[i] = vertex.get_texture_id();
		
		// Flattened Colors
		const glm::vec4 color = vertex.get_color();
		mFlattenedColors[i * 4 + 0] = color.r;
		mFlattenedColors[i * 4 + 1] = color.g;
		mFlattenedColors[i * 4 + 2] = color.b;
		mFlattenedColors[i * 4 + 3] = color.a;
		
		// Bone IDs and Weights
		const auto vertexBoneIds = vertex.get_bone_ids();
		const auto vertexWeights = vertex.get_weights();
		
		for (int j = 0; j < SkinnedMeshVertex::MAX_BONE_INFLUENCE; ++j) {
			mFlattenedBoneIds[i * SkinnedMeshVertex::MAX_BONE_INFLUENCE + j] = vertexBoneIds[j];
			mFlattenedWeights[i * SkinnedMeshVertex::MAX_BONE_INFLUENCE + j] = vertexWeights[j];
		}

	}
	
	mModelMatrix = nanogui::Matrix4f::identity();
	
	// Append the mesh to the batch
	mMeshBatch.add_mesh(*this);
	mMeshBatch.append(*this);
}

SkinnedMesh::~SkinnedMesh() {
	mMeshBatch.remove(*this);
}

void SkinnedMesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							   const nanogui::Matrix4f& projection) {
	
	mModelMatrix = model;
}
