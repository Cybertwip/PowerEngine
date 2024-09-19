#include "Mesh.hpp"

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "components/ColorComponent.hpp"
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

Mesh::SkinnedMeshShader::SkinnedMeshShader(ShaderManager& shaderManager)
: ShaderWrapper(*shaderManager.get_shader("mesh")) {}

void Mesh::MeshBatch::upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
#if defined(NANOGUI_USE_METAL)
	// Ensure we have a valid number of materials
	size_t numMaterials = materialData.size();
	
	// Create a CPU-side array of Material structs
	std::vector<nanogui::Texture*> texturesCPU(numMaterials);
	std::vector<MaterialCPU> materialsCPU(numMaterials);

	for (size_t i = 0; i < numMaterials; ++i) {
		auto& material = *materialData[i];
		MaterialCPU& materialCPU = materialsCPU[i];
		
		// Copy ambient, diffuse, and specular as arrays
		memcpy(&materialCPU.mAmbient[0], glm::value_ptr(material.mAmbient), sizeof(materialCPU.mAmbient));
		
		memcpy(&materialCPU.mDiffuse[0], glm::value_ptr(material.mDiffuse), sizeof(materialCPU.mDiffuse));
		
		memcpy(&materialCPU.mSpecular[0], glm::value_ptr(material.mSpecular), sizeof(materialCPU.mSpecular));
		
		// Copy the rest of the individual fields
		materialCPU.mShininess = material.mShininess;
		materialCPU.mOpacity = material.mOpacity;
		materialCPU.mHasDiffuseTexture = material.mHasDiffuseTexture ? 1.0f : 0.0f;

		// Set the diffuse texture (if it exists) or a dummy texture
		std::string textureBaseName = "textures";
		if (material.mHasDiffuseTexture) {
			shader.set_texture(textureBaseName, *material.mTextureDiffuse, i);
		} else {
			shader.set_texture(textureBaseName, *mDummyTexture, i);
		}

	}
	
	shader.set_buffer(
					   "materials",
					   nanogui::VariableType::Float32,
					   {numMaterials, sizeof(MaterialCPU) / sizeof(float)},
					   materialsCPU.data()
					   );

	size_t dummy_texture_count = shader.get_buffer_size("textures");
	
	for (size_t i = numMaterials; i<dummy_texture_count; ++i) {
		std::string textureBaseName = "textures";

		shader.set_texture(textureBaseName, *mDummyTexture, i);
	}

#else
	for (int i = 0; i < materialData.size(); ++i) {
		// Create the uniform name dynamically for each material (e.g., "materials[0].ambient")
		auto& material = *materialData[i];
		std::string baseName = "materials[" + std::to_string(i) + "].";
		
		// Uploading vec3 uniforms for each material (ambient, diffuse, specular)
		mShader.set_uniform(baseName + "ambient",
							nanogui::Vector3f(material.mAmbient.x, material.mAmbient.y, material.mAmbient.z));
		mShader.set_uniform(baseName + "diffuse",
							nanogui::Vector3f(material.mDiffuse.x, material.mDiffuse.y, material.mDiffuse.z));
		mShader.set_uniform(baseName + "specular",
							nanogui::Vector3f(material.mSpecular.x, material.mSpecular.y, material.mSpecular.z));
		
		// Upload float uniforms for shininess and opacity
		mShader.set_uniform(baseName + "shininess", material.mShininess);
		mShader.set_uniform(baseName + "opacity", material.mOpacity);
		
		// Convert boolean to integer for setting in the shader (0 or 1)
		mShader.set_uniform(baseName + "diffuse_texture", material.mHasDiffuseTexture ? 1.0f : 0.0f);
		
		// Set the diffuse texture (if it exists) or a dummy texture
		std::string textureBaseName = "textures[" + std::to_string(i) + "]";
		if (material.mHasDiffuseTexture) {
			mShader.set_texture(textureBaseName, material.mTextureDiffuse.get());
		} else {
			mShader.set_texture(textureBaseName, mDummyTexture.get());
		}
	}
#endif
}


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

		// Bone IDs and Weights
		//			const auto& vertexBoneIds = vertex.get_bone_ids();
		//			const auto& vertexWeights = vertex.get_weights();
		//
		//			for (int j = 0; j < Vertex::MAX_BONE_INFLUENCE; ++j) {
		//				mFlattenedBoneIds[i * Vertex::MAX_BONE_INFLUENCE + j] = vertexBoneIds[j];
		//				mFlattenedWeights[i * Vertex::MAX_BONE_INFLUENCE + j] = vertexWeights[j];
		//			}
	}
	
	mModelMatrix = nanogui::Matrix4f::identity(); // Or any other transformation
	
//	mMeshBatch.add_mesh(*this);
//	mMeshBatch.append(*this);
}

void Mesh::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
							   const nanogui::Matrix4f& projection) {
	
	mModelMatrix = model;
	//
	//    using namespace nanogui;
	//
	//	mShader.set_buffer("indices", nanogui::VariableType::UInt32, {mMeshData->mIndices.size()},
	//					   mMeshData->mIndices.data());
	//	mShader.upload_vertex_data(*this);
	//	mShader.upload_material_data(mMeshData->mMaterials);
	//
	//    //
	//    // Calculate bounding box to center the model
	////    glm::vec3 minPos(std::numeric_limits<float>::max());
	////    glm::vec3 maxPos(std::numeric_limits<float>::lowest());
	////
	////	for (const auto& vertex : mMeshData->mVertices) {
	////        minPos = glm::min(minPos, vertex->get_position());
	////        maxPos = glm::max(maxPos, vertex->get_position());
	////    }
	////
	////    auto center = (minPos + maxPos) / 2.0f;
	////
	////    Matrix4f m = model * Matrix4f::rotate(Vector3f(0, 1, 0), (float)glfwGetTime()) *
	////                 Matrix4f::translate(-Vector3f(center.x, center.y, center.z));
	//
	//    mShader.set_uniform("aModel", model);
	//    mShader.set_uniform("aView", view);
	//	mShader.set_uniform("aProjection", projection);
	//
	//	mShader.begin();
	//	mShader.draw_array(Shader::PrimitiveType::Triangle, 0, mMeshData->mIndices.size(), true);
	//	mShader.end();
}

// Create a dummy 1x1 white texture using the Texture constructor

void Mesh::init_dummy_texture() {
	mDummyTexture = std::make_unique<nanogui::Texture>(
													   nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
													   nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
													   nanogui::Vector2i(1, 1));                              // Size of the texture (1x1)
	
}

std::unique_ptr<nanogui::Texture> Mesh::mDummyTexture;

Mesh::MeshBatch::MeshBatch(nanogui::RenderPass& renderPass) : mRenderPass(renderPass) {
}

void Mesh::MeshBatch::add_mesh(std::reference_wrapper<Mesh> mesh) {
	
	auto it = std::find_if(mMeshes.begin(), mMeshes.end(), [mesh](auto kp) {
		return kp.first->identifier() == mesh.get().get_shader().identifier();
	});

	if (it != mMeshes.end()) {
		it->second.push_back(mesh);
	} else {
		mMeshes[&(mesh.get().get_shader())].push_back(mesh);
	}
}

void Mesh::MeshBatch::clear() {
	mBatchPositions.clear();
	mBatchNormals.clear();
	mBatchTexCoords1.clear();
	mBatchTexCoords2.clear();
	mBatchTextureIds.clear();
	mBatchIndices.clear();
	mBatchMaterials.clear();
	mMeshStartIndices.clear();
	
}

void Mesh::MeshBatch::append(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	
	auto& shader = mesh.get_shader();
	int identifier = shader.identifier();
	auto& indexer = mVertexIndexingMap[identifier];
	
	// Append vertex data
	mBatchPositions[identifier].insert(mBatchPositions[identifier].end(),
						   mesh.mFlattenedPositions.begin(),
						   mesh.mFlattenedPositions.end());
	mBatchNormals[identifier].insert(mBatchNormals[identifier].end(),
						 mesh.mFlattenedNormals.begin(),
						 mesh.mFlattenedNormals.end());
	mBatchTexCoords1[shader.identifier()].insert(mBatchTexCoords1[shader.identifier()].end(),
							mesh.mFlattenedTexCoords1.begin(),
							mesh.mFlattenedTexCoords1.end());
	mBatchTexCoords2[shader.identifier()].insert(mBatchTexCoords2[shader.identifier()].end(),
							mesh.mFlattenedTexCoords2.begin(),
							mesh.mFlattenedTexCoords2.end());
	mBatchTextureIds[shader.identifier()].insert(mBatchTextureIds[shader.identifier()].end(),
							mesh.mFlattenedTextureIds.begin(),
							mesh.mFlattenedTextureIds.end());
	
	mBatchColors[shader.identifier()].insert(mBatchColors[shader.identifier()].end(),
											 mesh.mFlattenedColors.begin(),
											 mesh.mFlattenedColors.end());

	// Adjust and append indices
	for (auto index : mesh.mMeshData->mIndices) {
		mBatchIndices[shader.identifier()].push_back(index + indexer.mVertexOffset);
	}
	
	mMeshStartIndices[shader.identifier()].push_back(indexer.mIndexOffset);
	
	indexer.mIndexOffset += mesh.mMeshData->mIndices.size();
	indexer.mVertexOffset += mesh.mMeshData->mVertices.size();
	
	
	// Upload consolidated data to GPU
	shader.set_buffer("aPosition", nanogui::VariableType::Float32, {mBatchPositions[identifier].size() / 3, 3},
					  mBatchPositions[identifier].data());
	shader.set_buffer("aNormal", nanogui::VariableType::Float32, {mBatchNormals[identifier].size() / 3, 3},
					  mBatchNormals[identifier].data());
	shader.set_buffer("aTexcoords1", nanogui::VariableType::Float32, {mBatchTexCoords1[identifier].size() / 2, 2},
					  mBatchTexCoords1[identifier].data());
	shader.set_buffer("aTexcoords2", nanogui::VariableType::Float32, {mBatchTexCoords2[identifier].size() / 2, 2},
					  mBatchTexCoords2[identifier].data());
	shader.set_buffer("aTextureId", nanogui::VariableType::Int32, {mBatchTextureIds[identifier].size() / 2, 2},
					  mBatchTextureIds[identifier].data());
	
	// **Set Buffer for Vertex Colors**
	shader.set_buffer("aColor", nanogui::VariableType::Float32, {mBatchColors[identifier].size() / 4, 4},
					  mBatchColors[identifier].data());
	// Upload indices
	shader.set_buffer("indices", nanogui::VariableType::UInt32, {mBatchIndices[identifier].size()},
					  mBatchIndices[identifier].data());
}

void Mesh::MeshBatch::draw_content(const nanogui::Matrix4f& view,
										  const nanogui::Matrix4f& projection) {
	
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	// Enable stencil test
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	
	// Clear stencil buffer and depth buffer
	glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// First pass: Mark the stencil buffer
	glStencilFunc(GL_ALWAYS, 1, 0xFF);  // Always pass stencil test
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // Replace stencil buffer with 1 where actors are drawn
	glStencilMask(0xFF);  // Enable writing to the stencil buffer
#elif defined(NANOGUI_USE_METAL)
//	
//	auto descriptor = mShader.render_pass().pass_descriptor();
//	auto encoder = mShader.render_pass().command_encoder();
//
//	// Get the Metal device and command encoder (ensure you have valid pointers to these)
//	void* mtlDevice = nanogui::metal_device();
//	void* mtlCommandEncoder = encoder;
//	
//	// Create a depth-stencil state for this draw call
//	void* depthStencilState = MetalHelper::createDepthStencilState(mtlDevice, CompareFunction::Always,
//																   StencilOperation::Keep, StencilOperation::Keep,
//																   StencilOperation::Replace, 0xFF, 0xFF);
//	
//	// Set the depth-stencil state on the command encoder
//	MetalHelper::setDepthStencilState(mtlCommandEncoder, depthStencilState);
//	
//	// Clear stencil and depth buffers
//	MetalHelper::setStencilClear(descriptor);
//	MetalHelper::setDepthClear(descriptor);
#endif
	
	for (auto& [shader_pointer, mesh_vector] : mMeshes) {
		auto& shader = *shader_pointer;
		int identifier = shader.identifier();
		
		mRenderPass.pop_depth_test_state(identifier);
		
		for (int i = 0; i<mesh_vector.size(); ++i) {
			auto& mesh = mesh_vector[i].get();
			
			if (!mesh.mColorComponent.get_visible()) {
				continue;
			}
			
			auto& shader = mesh.get_shader();
			
			// Set uniforms and draw the mesh content
			shader.set_uniform("aView", view);
			shader.set_uniform("aProjection", projection);


			// Set the model matrix for the current mesh
			shader.set_uniform("aModel", mesh.mModelMatrix);

			// Apply color component (assuming it sets relevant uniforms)
			mesh.mColorComponent.apply_to(shader);
			
			// Upload materials for the current mesh
			upload_material_data(shader, mesh.mMeshData->mMaterials);
			
			// Calculate the range of indices to draw for this mesh
			size_t startIdx = mMeshStartIndices[identifier][i];
			size_t count = (i < mesh_vector.size() - 1) ?
			(mMeshStartIndices[identifier][i + 1] - startIdx) :
			(mBatchIndices[identifier].size() - startIdx);
			
			// Begin shader program
			shader.begin();
			
			// Draw the mesh segment
			shader.draw_array(nanogui::Shader::PrimitiveType::Triangle,
							  startIdx, count, true);
			
			// End shader program
			shader.end();
		}

	}


}
