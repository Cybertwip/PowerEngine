#include "SkinnedMesh.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/renderpass.h>
#include <nanogui/texture.h>

#include <cmath>

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "import/Fbx.hpp"

SkinnedMesh::Vertex::Vertex() {}

SkinnedMesh::Vertex::Vertex(const glm::vec3& pos, const glm::vec2& tex)
    : mPosition(pos), mNormal(0.0f), mTexCoords1(tex), mTexCoords2(tex) {
    init_bones();
}

void SkinnedMesh::Vertex::init_bones() {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        mBoneIds[i] = -1;
        mWeights[i] = 0.0f;
    }
}

void SkinnedMesh::Vertex::set_bone(int boneId, float weight) {
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

void SkinnedMesh::Vertex::set_position(const glm::vec3& vec) { mPosition = vec; }

void SkinnedMesh::Vertex::set_normal(const glm::vec3& vec) { mNormal = vec; }

void SkinnedMesh::Vertex::set_texture_coords1(const glm::vec2& vec) { mTexCoords1 = vec; }

void SkinnedMesh::Vertex::set_texture_coords2(const glm::vec2& vec) { mTexCoords2 = vec; }

void SkinnedMesh::Vertex::set_texture_id(int textureId) { mTextureId = textureId; }

glm::vec3 SkinnedMesh::Vertex::get_position() const { return mPosition; }

glm::vec3 SkinnedMesh::Vertex::get_normal() const { return mNormal; }

glm::vec2 SkinnedMesh::Vertex::get_tex_coords1() const { return mTexCoords1; }

glm::vec2 SkinnedMesh::Vertex::get_tex_coords2() const { return mTexCoords2; }

std::array<int, SkinnedMesh::Vertex::MAX_BONE_INFLUENCE> SkinnedMesh::Vertex::get_bone_ids() const {
    return mBoneIds;
}
std::array<float, SkinnedMesh::Vertex::MAX_BONE_INFLUENCE> SkinnedMesh::Vertex::get_weights()
    const {
    return mWeights;
}

int SkinnedMesh::Vertex::get_texture_id() const {
	return mTextureId;
}

SkinnedMesh::SkinnedMeshShader::SkinnedMeshShader(ShaderManager& shaderManager)
    : ShaderWrapper(*shaderManager.get_shader("mesh")) {}

void SkinnedMesh::SkinnedMeshShader::upload_vertex_data(const SkinnedMesh& skinnedMesh) {
	size_t numVertices = skinnedMesh.mFlattenedPositions.size() / 3;
	
	mShader.set_buffer("aPosition", nanogui::VariableType::Float32, {numVertices, 3},
					   skinnedMesh.mFlattenedPositions.data());
	mShader.set_buffer("aNormal", nanogui::VariableType::Float32, {numVertices, 3},
					   skinnedMesh.mFlattenedNormals.data());
	mShader.set_buffer("aTexcoords1", nanogui::VariableType::Float32, {numVertices, 2},
					   skinnedMesh.mFlattenedTexCoords1.data());
	mShader.set_buffer("aTexcoords2", nanogui::VariableType::Float32, {numVertices, 2},
					   skinnedMesh.mFlattenedTexCoords2.data());
	mShader.set_buffer("aTextureId", nanogui::VariableType::Int32, {numVertices, 2},
					   skinnedMesh.mFlattenedTextureIds.data());
	
}

void SkinnedMesh::SkinnedMeshShader::upload_material_data(const std::vector<MaterialProperties>& materialData) {
	for (int i = 0; i < materialData.size(); ++i) {
		// Create the uniform name dynamically for each material (e.g., "materials[0].ambient")
		std::string baseName = "materials[" + std::to_string(i) + "].";
		
		// Uploading vec3 uniforms for each material
		mShader.set_uniform(baseName + "ambient",
							nanogui::Vector3f(materialData[i].mAmbient.x, materialData[i].mAmbient.y,
											  materialData[i].mAmbient.z));
		mShader.set_uniform(baseName + "diffuse",
							nanogui::Vector3f(materialData[i].mDiffuse.x, materialData[i].mDiffuse.y,
											  materialData[i].mDiffuse.z));
		mShader.set_uniform(baseName + "specular",
							nanogui::Vector3f(materialData[i].mSpecular.x, materialData[i].mSpecular.y,
											  materialData[i].mSpecular.z));
		
		// Uploading float uniforms for shininess and opacity
		mShader.set_uniform(baseName + "shininess", materialData[i].mShininess);
		mShader.set_uniform(baseName + "opacity", materialData[i].mOpacity);
		
		// Uploading boolean for texture presence
		mShader.set_uniform(baseName + "has_diffuse_texture", materialData[i].mHasDiffuseTexture);

		if (materialData[i].mHasDiffuseTexture) {
			mShader.set_texture(baseName + "texture_diffuse", materialData[i].mTextureDiffuse.get());
		} else {
			mShader.set_texture(baseName + "texture_diffuse", mDummyTexture.get());
		}
	}
}

SkinnedMesh::SkinnedMesh(std::unique_ptr<MeshData> meshData, SkinnedMeshShader& shader)
    : mMeshData(std::move(meshData)), mShader(shader) {
		
		size_t numVertices = mMeshData->mVertices.size();
		
		// Pre-allocate flattened data vectors
		mFlattenedPositions.resize(numVertices * 3);
		mFlattenedNormals.resize(numVertices * 3);
		mFlattenedTexCoords1.resize(numVertices * 2);
		mFlattenedTexCoords2.resize(numVertices * 2);
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
			
			// Bone IDs and Weights
//			const auto& vertexBoneIds = vertex.get_bone_ids();
//			const auto& vertexWeights = vertex.get_weights();
//			
//			for (int j = 0; j < Vertex::MAX_BONE_INFLUENCE; ++j) {
//				mFlattenedBoneIds[i * Vertex::MAX_BONE_INFLUENCE + j] = vertexBoneIds[j];
//				mFlattenedWeights[i * Vertex::MAX_BONE_INFLUENCE + j] = vertexWeights[j];
//			}
		}

}

void SkinnedMesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                               const nanogui::Matrix4f& projection) {
    using namespace nanogui;
	
	mShader.set_buffer("indices", nanogui::VariableType::UInt32, {mMeshData->mIndices.size()},
					   mMeshData->mIndices.data());
	mShader.upload_vertex_data(*this);
	mShader.upload_material_data(mMeshData->mMaterials);

    //
    // Calculate bounding box to center the model
//    glm::vec3 minPos(std::numeric_limits<float>::max());
//    glm::vec3 maxPos(std::numeric_limits<float>::lowest());
//
//	for (const auto& vertex : mMeshData->mVertices) {
//        minPos = glm::min(minPos, vertex->get_position());
//        maxPos = glm::max(maxPos, vertex->get_position());
//    }
//
//    auto center = (minPos + maxPos) / 2.0f;
//
//    Matrix4f m = model * Matrix4f::rotate(Vector3f(0, 1, 0), (float)glfwGetTime()) *
//                 Matrix4f::translate(-Vector3f(center.x, center.y, center.z));

    mShader.set_uniform("aModel", model);
    mShader.set_uniform("aView", view);
	mShader.set_uniform("aProjection", projection);

	mShader.begin();
	mShader.draw_array(Shader::PrimitiveType::Triangle, 0, mMeshData->mIndices.size(), true);
	mShader.end();
}

// Create a dummy 1x1 white texture using the Texture constructor

void SkinnedMesh::init_dummy_texture() {
	mDummyTexture = std::make_unique<nanogui::Texture>(
										 nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
										 nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
										 nanogui::Vector2i(1, 1));                              // Size of the texture (1x1)

}

std::unique_ptr<nanogui::Texture> SkinnedMesh::mDummyTexture;
