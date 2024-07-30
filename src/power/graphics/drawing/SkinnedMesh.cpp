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

SkinnedMesh::SkinnedMeshShader::SkinnedMeshShader(ShaderManager& shaderManager)
    : ShaderWrapper(*shaderManager.get_shader("mesh")) {}

void SkinnedMesh::SkinnedMeshShader::upload_vertex_data(
    const std::vector<std::unique_ptr<Vertex>>& vertexData) {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords1;
    std::vector<float> texCoords2;
    std::vector<int> boneIds;
    std::vector<float> weights;
    for (const auto& vertexPointer : vertexData) {
        auto& vertex = *vertexPointer;
        positions.insert(positions.end(), {vertex.get_position().x, vertex.get_position().y,
                                           vertex.get_position().z});
        normals.insert(normals.end(),
                       {vertex.get_normal().x, vertex.get_normal().y, vertex.get_normal().z});
        texCoords1.insert(texCoords1.end(),
                          {vertex.get_tex_coords1().x, vertex.get_tex_coords1().y});
        texCoords2.insert(texCoords2.end(),
                          {vertex.get_tex_coords2().x, vertex.get_tex_coords2().y});

        auto vertexBoneIds = vertex.get_bone_ids();
        auto vertexWeights = vertex.get_weights();

        boneIds.insert(boneIds.end(), vertexBoneIds.begin(), vertexBoneIds.end());
        weights.insert(weights.end(), vertexWeights.begin(), vertexWeights.end());
    }

    mShader.set_buffer("aPosition", nanogui::VariableType::Float32, {vertexData.size(), 3},
                       positions.data());
    mShader.set_buffer("aNormal", nanogui::VariableType::Float32, {vertexData.size(), 3},
                       normals.data());
    mShader.set_buffer("aTexcoords1", nanogui::VariableType::Float32, {vertexData.size(), 2},
                       texCoords1.data());
    mShader.set_buffer("aTexcoords2", nanogui::VariableType::Float32, {vertexData.size(), 2},
                       texCoords2.data());
    // mShader.set_buffer("boneIds", nanogui::VariableType::Int32, {vertexData.size(),
    // MAX_BONE_INFLUENCE}, boneIds.data()); mShader.set_buffer("weights",
    // nanogui::VariableType::Float32, {vertexData.size(), MAX_BONE_INFLUENCE}, weights.data());
}

void SkinnedMesh::SkinnedMeshShader::upload_material_data(const MaterialProperties& materialData) {
    // Uploading vec3 uniforms
    mShader.set_uniform("material.ambient",
                        nanogui::Vector3f(materialData.mAmbient.x, materialData.mAmbient.y,
                                          materialData.mAmbient.z));
    mShader.set_uniform("material.diffuse",
                        nanogui::Vector3f(materialData.mDiffuse.x, materialData.mDiffuse.y,
                                          materialData.mDiffuse.z));
    mShader.set_uniform("material.specular",
                        nanogui::Vector3f(materialData.mSpecular.x, materialData.mSpecular.y,
                                          materialData.mSpecular.z));

    // Uploading float uniforms
    mShader.set_uniform("material.shininess", materialData.mShininess);
    mShader.set_uniform("material.opacity", materialData.mOpacity);

    // Uploading boolean uniform
    mShader.set_uniform("material.has_diffuse_texture", materialData.mHasDiffuseTexture);
}

void SkinnedMesh::SkinnedMeshShader::upload_texture_data(
    std::vector<std::unique_ptr<nanogui::Texture>>& textureData) {
    mShader.set_texture("texture_diffuse1", textureData[0].get());
}

SkinnedMesh::SkinnedMesh(std::unique_ptr<MeshData> meshData, SkinnedMeshShader& shader)
    : mMeshData(std::move(meshData)), mShader(shader) {
    initialize_mesh();
}

void SkinnedMesh::initialize_mesh() {
	mShader.upload_index_data(mMeshData->mIndices);
	mShader.upload_vertex_data(mMeshData->mVertices);
	mShader.upload_material_data(mMeshData->mMaterial);
	if (mMeshData->mMaterial.mHasDiffuseTexture) {
		mShader.upload_texture_data(mMeshData->mTextures);
    }
}

void SkinnedMesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                               const nanogui::Matrix4f& projection) {
    using namespace nanogui;
	
	mShader.upload_vertex_data(mMeshData->mVertices);
	mShader.upload_material_data(mMeshData->mMaterial);
	if (mMeshData->mMaterial.mHasDiffuseTexture) {
		mShader.upload_texture_data(mMeshData->mTextures);
	}

    //
    // Calculate bounding box to center the model
    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

	for (const auto& vertex : mMeshData->mVertices) {
        minPos = glm::min(minPos, vertex->get_position());
        maxPos = glm::max(maxPos, vertex->get_position());
    }

    auto center = (minPos + maxPos) / 2.0f;

    Matrix4f m = model * Matrix4f::rotate(Vector3f(0, 1, 0), (float)glfwGetTime()) *
                 Matrix4f::translate(-Vector3f(center.x, center.y, center.z));

    mShader.set_uniform("aModel", m);
    mShader.set_uniform("aView", view);
	mShader.set_uniform("aProjection", projection);

	mShader.begin();
	mShader.draw_array(Shader::PrimitiveType::Triangle, 0, mMeshData->mIndices.size(), true);
	mShader.end();
}
