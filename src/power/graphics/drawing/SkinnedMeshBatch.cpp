#include "SkinnedMeshBatch.hpp"

#include "components/ColorComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <glm/gtc/type_ptr.hpp>

void SkinnedMeshBatch::upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
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
			shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
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
		
		shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
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


SkinnedMeshBatch::SkinnedMeshBatch(nanogui::RenderPass& renderPass) : mRenderPass(renderPass) {
}

void SkinnedMeshBatch::add_mesh(std::reference_wrapper<SkinnedMesh> mesh) {
	
	auto it = std::find_if(mMeshes.begin(), mMeshes.end(), [mesh](auto kp) {
		return kp.first->identifier() == mesh.get().get_shader().identifier();
	});
	
	if (it != mMeshes.end()) {
		it->second.push_back(mesh);
	} else {
		mMeshes[&(mesh.get().get_shader())].push_back(mesh);
	}
}

void SkinnedMeshBatch::clear() {
	mBatchPositions.clear();
	mBatchNormals.clear();
	mBatchTexCoords1.clear();
	mBatchTexCoords2.clear();
	mBatchTextureIds.clear();
	mBatchIndices.clear();
	mBatchMaterials.clear();
	mMeshStartIndices.clear();
}

void SkinnedMeshBatch::append(std::reference_wrapper<SkinnedMesh> meshRef) {
	auto& mesh = meshRef.get();
	
	auto& shader = mesh.get_shader();
	int identifier = shader.identifier();
	auto& indexer = mVertexIndexingMap[identifier];
	
	// Append vertex data using getters
	mBatchPositions[identifier].insert(mBatchPositions[identifier].end(),
									   mesh.get_flattened_positions().begin(),
									   mesh.get_flattened_positions().end());
	mBatchNormals[identifier].insert(mBatchNormals[identifier].end(),
									 mesh.get_flattened_normals().begin(),
									 mesh.get_flattened_normals().end());
	mBatchTexCoords1[shader.identifier()].insert(mBatchTexCoords1[shader.identifier()].end(),
												 mesh.get_flattened_tex_coords1().begin(),
												 mesh.get_flattened_tex_coords1().end());
	mBatchTexCoords2[shader.identifier()].insert(mBatchTexCoords2[shader.identifier()].end(),
												 mesh.get_flattened_tex_coords2().begin(),
												 mesh.get_flattened_tex_coords2().end());
	mBatchTextureIds[shader.identifier()].insert(mBatchTextureIds[shader.identifier()].end(),
												 mesh.get_flattened_texture_ids().begin(),
												 mesh.get_flattened_texture_ids().end());
	
	mBatchColors[shader.identifier()].insert(mBatchColors[shader.identifier()].end(),
											 mesh.get_flattened_colors().begin(),
											 mesh.get_flattened_colors().end());
	
	mBatchBoneIds[shader.identifier()].insert(mBatchBoneIds[shader.identifier()].end(),
mesh.get_flattened_bone_ids().begin(),
mesh.get_flattened_bone_ids().end());

	
	mBatchBoneWeights[shader.identifier()].insert(mBatchBoneWeights[shader.identifier()].end(),
											  mesh.get_flattened_weights().begin(),
											  mesh.get_flattened_weights().end());

	// Adjust and append indices
	auto& indices = mesh.get_mesh_data().get_indices()
	;
	for (auto index : indices) {
		mBatchIndices[shader.identifier()].push_back(index + indexer.mVertexOffset);
	}
	
	mMeshStartIndices[shader.identifier()].push_back(indexer.mIndexOffset);
	
	indexer.mIndexOffset += mesh.get_mesh_data().get_indices().size();
	indexer.mVertexOffset += mesh.get_mesh_data().get_skinned_vertices().size();
	
	upload_vertex_data(shader, identifier);
}

void SkinnedMeshBatch::upload_vertex_data(ShaderWrapper& shader, int identifier) {
	// Upload consolidated data to GPU
	shader.persist_buffer("aPosition", nanogui::VariableType::Float32, {mBatchPositions[identifier].size() / 3, 3},
					  mBatchPositions[identifier].data());
	shader.persist_buffer("aNormal", nanogui::VariableType::Float32, {mBatchNormals[identifier].size() / 3, 3},
					  mBatchNormals[identifier].data());
	shader.persist_buffer("aTexcoords1", nanogui::VariableType::Float32, {mBatchTexCoords1[identifier].size() / 2, 2},
					  mBatchTexCoords1[identifier].data());
	shader.persist_buffer("aTexcoords2", nanogui::VariableType::Float32, {mBatchTexCoords2[identifier].size() / 2, 2},
					  mBatchTexCoords2[identifier].data());
	shader.persist_buffer("aTextureId", nanogui::VariableType::Int32, {mBatchTextureIds[identifier].size(), 1},
					  mBatchTextureIds[identifier].data());
	
	// Set Buffer for Vertex Colors
	shader.persist_buffer("aColor", nanogui::VariableType::Float32, {mBatchColors[identifier].size() / 4, 4},
					  mBatchColors[identifier].data());
	
	// Set Buffer for Bone IDs
	shader.persist_buffer("aBoneIds", nanogui::VariableType::Int32, {mBatchBoneIds[identifier].size() / 4, 4},
					  mBatchBoneIds[identifier].data());
	
	// Set Buffer for Weights
	shader.persist_buffer("aWeights", nanogui::VariableType::Float32, {mBatchBoneWeights[identifier].size() / 4, 4},
					  mBatchBoneWeights[identifier].data());
	
	// Upload indices
	shader.persist_buffer("indices", nanogui::VariableType::UInt32, {mBatchIndices[identifier].size()},
					  mBatchIndices[identifier].data());
}

void SkinnedMeshBatch::draw_content(const nanogui::Matrix4f& view,
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
#endif
	
	for (auto& [shader_pointer, mesh_vector] : mMeshes) {
		auto& shader = *shader_pointer;
		int identifier = shader.identifier();
				
		mRenderPass.pop_depth_test_state(identifier);
		
		for (int i = 0; i<mesh_vector.size(); ++i) {
			auto& mesh = mesh_vector[i].get();
			
			if (!mesh.get_color_component().get_visible()) {
				continue;
			}
			
			auto& shader = mesh.get_shader();
			
			// Set uniforms and draw the mesh content
			shader.set_uniform("aView", view);
			shader.set_uniform("aProjection", projection);
			
			// Set the model matrix for the current mesh
			shader.set_uniform("aModel", mesh.get_model_matrix());
			
			// Apply skinning and animations (if any)
			auto bones = mesh.get_skinned_component().get_bones();

			shader.set_buffer(
							  "bones",
							  nanogui::VariableType::Float32,
							  {bones.size(), sizeof(SkinnedAnimationComponent::BoneCPU) / sizeof(float)},
							  bones.data(), 11);
			// Apply color component (assuming it sets relevant uniforms)
			mesh.get_color_component().apply_to(shader);
			
			// Upload materials for the current mesh
			upload_material_data(shader, mesh.get_mesh_data().get_material_properties());
			
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
