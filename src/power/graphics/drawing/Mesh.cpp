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

Mesh::Vertex::Vertex() {}

Mesh::Vertex::Vertex(const glm::vec3& pos, const glm::vec2& tex)
: mPosition(pos), mNormal(0.0f), mTexCoords1(tex), mTexCoords2(tex) {
	init_bones();
}

void Mesh::Vertex::init_bones() {
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
		mBoneIds[i] = -1;
		mWeights[i] = 0.0f;
	}
}

void Mesh::Vertex::set_bone(int boneId, float weight) {
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
		if (mBoneIds[i] < 0) {
			mBoneIds[i] = boneId;
			mWeights[i] = weight;
			return;
		}
	}
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
		if (mWeights[i] < weight) {
			mBoneIds[i] = boneId;
			mWeights[i] = weight;
			return;
		}
	}
}

void Mesh::Vertex::set_position(const glm::vec3& vec) { mPosition = vec; }

void Mesh::Vertex::set_normal(const glm::vec3& vec) { mNormal = vec; }
void Mesh::Vertex::set_color(const glm::vec4& vec) { mColor = vec; }

void Mesh::Vertex::set_texture_coords1(const glm::vec2& vec) { mTexCoords1 = vec; }

void Mesh::Vertex::set_texture_coords2(const glm::vec2& vec) { mTexCoords2 = vec; }

void Mesh::Vertex::set_texture_id(int textureId) { mTextureId = textureId; }

glm::vec3 Mesh::Vertex::get_position() const { return mPosition; }

glm::vec3 Mesh::Vertex::get_normal() const { return mNormal; }

glm::vec4 Mesh::Vertex::get_color() const { return mColor; }

glm::vec2 Mesh::Vertex::get_tex_coords1() const { return mTexCoords1; }

glm::vec2 Mesh::Vertex::get_tex_coords2() const { return mTexCoords2; }

std::array<int, Mesh::Vertex::MAX_BONE_INFLUENCE> Mesh::Vertex::get_bone_ids() const {
	return mBoneIds;
}
std::array<float, Mesh::Vertex::MAX_BONE_INFLUENCE> Mesh::Vertex::get_weights()
const {
	return mWeights;
}

int Mesh::Vertex::get_texture_id() const {
	return mTextureId;
}

Mesh::MeshShader::MeshShader(ShaderManager& shaderManager)
: ShaderWrapper(*shaderManager.get_shader("mesh")) {}

Mesh::Mesh(std::unique_ptr<MeshData> meshData, ShaderWrapper& shader, MeshBatch& meshBatch, ColorComponent& colorComponent)
: mMeshData(std::move(meshData)), mShader(shader), mMeshBatch(meshBatch), mColorComponent(colorComponent), mModelMatrix(nanogui::Matrix4f::identity()) {
	size_t numVertices = mMeshData->mVertices.size();
	
	// Pre-allocate flattened data vectors
	mFlattenedPositions.resize(numVertices * 3);
	mFlattenedNormals.resize(numVertices * 3);
	mFlattenedTexCoords1.resize(numVertices * 2);
	mFlattenedTexCoords2.resize(numVertices * 2);
	mFlattenedColors.resize(numVertices * 4); // vec4 per vertex

	//		mFlattenedBoneIds.resize(numVertices * Vertex::MAX_BONE_INFLUENCE);
	//		mFlattenedWeights.resize(numVertices * Vertex::MAX_BONE_INFLUENCE);
	mFlattenedTextureIds.resize(numVertices * 2); // Assuming two texture IDs per vertex
	
	// Flatten the vertex data
	for (size_t i = 0; i < numVertices; ++i) {
		const auto& vertex = mMeshData->mVertices[i];
		
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
		mFlattenedTextureIds[i * 2 + 0] = vertex.get_texture_id();
		mFlattenedTextureIds[i * 2 + 1] = vertex.get_texture_id();
		
		// **Flattened Colors**
		const glm::vec4& color = vertex.get_color();
		mFlattenedColors[i * 4 + 0] = color.r;
		mFlattenedColors[i * 4 + 1] = color.g;
		mFlattenedColors[i * 4 + 2] = color.b;
		mFlattenedColors[i * 4 + 3] = color.a;
	}
	
	mModelMatrix = nanogui::Matrix4f::identity(); // Or any other transformation
	
	mMeshBatch.add_mesh(*this);
	mMeshBatch.append(*this);
}

void Mesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							   const nanogui::Matrix4f& projection) {
	
	mModelMatrix = model;
}
