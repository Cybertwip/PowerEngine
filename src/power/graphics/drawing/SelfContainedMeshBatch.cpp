#include "SelfContainedMeshBatch.hpp"

#include "components/ColorComponent.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include <cstring>

SelfContainedMeshBatch::SelfContainedMeshBatch(nanogui::RenderPass& renderPass, ShaderWrapper& shader)
: mRenderPass(renderPass), mShader(shader) {
}

void SelfContainedMeshBatch::upload_material_data(const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
	size_t numMaterials = materialData.size();
	std::vector<MaterialCPU> materialsCPU(numMaterials);
	
	int textureSetCount = 0;
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
		
		// Set the diffuse texture (if it exists)
		std::string textureBaseName = "textures";
		if (material.mHasDiffuseTexture) {
			mShader.set_texture(textureBaseName, material.mTextureDiffuse, i);
			textureSetCount++;
		}
	}
	
	if (numMaterials == 0) { // Upload dummy material
		numMaterials = 1;
		materialsCPU.push_back({});
	}
	
	mShader.set_buffer(
					   "materials",
					   nanogui::VariableType::Float32,
					   { numMaterials, sizeof(MaterialCPU) / sizeof(float) },
					   materialsCPU.data()
					   );
	
	size_t dummy_texture_count = mShader.get_buffer_size("textures");
	
	for (size_t i = textureSetCount; i < dummy_texture_count; ++i) {
		std::string textureBaseName = "textures";
		mShader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
	}
}

void SelfContainedMeshBatch::add_mesh(std::reference_wrapper<Mesh> mesh) {
	mMeshes.push_back(mesh);
	append(mesh);
}

void SelfContainedMeshBatch::clear() {
	mMeshes.clear();
	mBatchPositions.clear();
	mBatchNormals.clear();
	mBatchColors.clear();
	mBatchTexCoords1.clear();
	mBatchTexCoords2.clear();
	mBatchMaterialIds.clear();
	mBatchIndices.clear();
	mMeshStartIndices.clear();
	mMeshVertexStartIndices.clear();
	mIndexer = Indexer{}; // Reset indexer
}

void SelfContainedMeshBatch::append(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	
	// Record starting indices before appending
	size_t vertexOffset = mIndexer.mVertexOffset;
	size_t indexOffset = mIndexer.mIndexOffset;
	
	mMeshStartIndices.push_back(indexOffset);
	mMeshVertexStartIndices.push_back(vertexOffset);
	
	// Append vertex data using getters
	mBatchPositions.insert(mBatchPositions.end(),
						   mesh.get_flattened_positions().begin(),
						   mesh.get_flattened_positions().end());
	mBatchNormals.insert(mBatchNormals.end(),
						 mesh.get_flattened_normals().begin(),
						 mesh.get_flattened_normals().end());
	mBatchTexCoords1.insert(mBatchTexCoords1.end(),
							mesh.get_flattened_tex_coords1().begin(),
							mesh.get_flattened_tex_coords1().end());
	mBatchTexCoords2.insert(mBatchTexCoords2.end(),
							mesh.get_flattened_tex_coords2().begin(),
							mesh.get_flattened_tex_coords2().end());
	mBatchMaterialIds.insert(mBatchMaterialIds.end(),
							 mesh.get_flattened_material_ids().begin(),
							 mesh.get_flattened_material_ids().end());
	mBatchColors.insert(mBatchColors.end(),
						mesh.get_flattened_colors().begin(),
						mesh.get_flattened_colors().end());
	
	// Adjust and append indices
	auto& indices = mesh.get_mesh_data().get_indices();
	for (auto index : indices) {
		mBatchIndices.push_back(index + vertexOffset);
	}
	
	// Update indexer
	mIndexer.mIndexOffset += indices.size();
	mIndexer.mVertexOffset += mesh.get_mesh_data().get_vertices().size();
	
	upload_vertex_data();
}

void SelfContainedMeshBatch::upload_vertex_data() {
	
	if (mMeshes.empty()) {
		return;
	}
	
	// Upload consolidated data to GPU
	mShader.persist_buffer("aPosition", nanogui::VariableType::Float32, { mBatchPositions.size() / 3, 3 },
						   mBatchPositions.data());
	mShader.persist_buffer("aNormal", nanogui::VariableType::Float32, { mBatchNormals.size() / 3, 3 },
						   mBatchNormals.data());
	mShader.persist_buffer("aTexcoords1", nanogui::VariableType::Float32, { mBatchTexCoords1.size() / 2, 2 },
						   mBatchTexCoords1.data());
	mShader.persist_buffer("aTexcoords2", nanogui::VariableType::Float32, { mBatchTexCoords2.size() / 2, 2 },
						   mBatchTexCoords2.data());
	mShader.persist_buffer("aMaterialId", nanogui::VariableType::Int32, { mBatchMaterialIds.size(), 1 },
						   mBatchMaterialIds.data());
	mShader.persist_buffer("aColor", nanogui::VariableType::Float32, { mBatchColors.size() / 4, 4 },
						   mBatchColors.data());
	mShader.persist_buffer("indices", nanogui::VariableType::UInt32, { mBatchIndices.size() },
						   mBatchIndices.data());
}

void SelfContainedMeshBatch::remove(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	
	// Find the mesh in mMeshes
	auto it = std::find_if(mMeshes.begin(), mMeshes.end(),
						   [&mesh](const std::reference_wrapper<Mesh>& m) {
		return &m.get() == &mesh;
	});
	
	if (it == mMeshes.end()) {
		// Mesh not found; nothing to remove
		return;
	}
	
	// Determine the index of the mesh within the vector
	size_t meshIndex = std::distance(mMeshes.begin(), it);
	
	// Remove the mesh from the mesh vector
	mMeshes.erase(it);
	
	// Get starting indices
	size_t startIdx = mMeshStartIndices[meshIndex];
	size_t vertexStartIdx = mMeshVertexStartIndices[meshIndex];
	
	size_t endIdx = (meshIndex + 1 < mMeshStartIndices.size()) ?
	mMeshStartIndices[meshIndex + 1] :
	mBatchIndices.size();
	size_t count = endIdx - startIdx;
	
	// Remove indices
	mBatchIndices.erase(
						mBatchIndices.begin() + startIdx,
						mBatchIndices.begin() + endIdx
						);
	
	// Adjust indices after removed indices
	size_t numVertices = mesh.get_mesh_data().get_vertices().size();
	
	for (auto& index : mBatchIndices) {
		if (index >= vertexStartIdx + numVertices) {
			index -= numVertices;
		} else if (index >= vertexStartIdx) {
			index = 0; // Or handle appropriately
		}
	}
	
	// Remove per-vertex data
	mBatchPositions.erase(
						  mBatchPositions.begin() + vertexStartIdx * 3,
						  mBatchPositions.begin() + (vertexStartIdx + numVertices) * 3
						  );
	mBatchNormals.erase(
						mBatchNormals.begin() + vertexStartIdx * 3,
						mBatchNormals.begin() + (vertexStartIdx + numVertices) * 3
						);
	mBatchTexCoords1.erase(
						   mBatchTexCoords1.begin() + vertexStartIdx * 2,
						   mBatchTexCoords1.begin() + (vertexStartIdx + numVertices) * 2
						   );
	mBatchTexCoords2.erase(
						   mBatchTexCoords2.begin() + vertexStartIdx * 2,
						   mBatchTexCoords2.begin() + (vertexStartIdx + numVertices) * 2
						   );
	mBatchMaterialIds.erase(
							mBatchMaterialIds.begin() + vertexStartIdx,
							mBatchMaterialIds.begin() + (vertexStartIdx + numVertices)
							);
	mBatchColors.erase(
					   mBatchColors.begin() + vertexStartIdx * 4,
					   mBatchColors.begin() + (vertexStartIdx + numVertices) * 4
					   );
	
	// Remove from mMeshStartIndices and mMeshVertexStartIndices
	mMeshStartIndices.erase(mMeshStartIndices.begin() + meshIndex);
	mMeshVertexStartIndices.erase(mMeshVertexStartIndices.begin() + meshIndex);
	
	// Adjust subsequent start indices and vertex start indices
	for (size_t i = meshIndex; i < mMeshStartIndices.size(); ++i) {
		mMeshStartIndices[i] -= count;
		mMeshVertexStartIndices[i] -= numVertices;
	}
	
	// Update indexer
	mIndexer.mVertexOffset -= numVertices;
	mIndexer.mIndexOffset -= count;
	
	// Re-upload data
	upload_vertex_data();
}

void SelfContainedMeshBatch::draw_content(const nanogui::Matrix4f& view,
										  const nanogui::Matrix4f& projection) {
	
	for (size_t i = 0; i < mMeshes.size(); ++i) {
		auto& mesh = mMeshes[i].get();
		
		if (!mesh.get_color_component().get_visible()) {
			continue;
		}
		
		// Set uniforms and draw the mesh content
		mShader.set_uniform("aProjection", projection);
		mShader.set_uniform("aView", view);
		mShader.set_uniform("aModel", mesh.get_model_matrix());
		
		// Apply color component
		mShader.set_uniform("identifier", mesh.get_color_component().identifier());
		
		mShader.set_uniform("color", glm_to_nanogui(mesh.get_color_component().get_color()));

		// Upload materials for the current mesh
		upload_material_data(mesh.get_mesh_data().get_material_properties());
		
		// Calculate the range of indices to draw for this mesh
		size_t startIdx = mMeshStartIndices[i];
		size_t count = (i + 1 < mMeshStartIndices.size()) ?
		(mMeshStartIndices[i + 1] - startIdx) :
		(mBatchIndices.size() - startIdx);
		
		// Begin shader program
		mShader.begin();
		
		// Draw the mesh segment
		mShader.draw_array(nanogui::Shader::PrimitiveType::Triangle,
						   startIdx, count, true);
		
		// End shader program
		mShader.end();
	}
}
