#include "MeshBatch.hpp"

#include "components/ColorComponent.hpp"

#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <glm/gtc/type_ptr.hpp>

void MeshBatch::upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
	// Ensure we have a valid number of materials
	size_t numMaterials = materialData.size();
	
	// Create a CPU-side array of Material structs
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
		
		// Set the diffuse texture (if it exists) or a dummy texture
		std::string textureBaseName = "textures";
		if (material.mHasDiffuseTexture) {
			shader.set_texture(textureBaseName, material.mTextureDiffuse, i);
			textureSetCount++;
		}
	}
	
	if (numMaterials == 0) { // upload dummy material
		numMaterials = 1;
		materialsCPU.push_back({});
	}
	
	shader.set_buffer(
					  "materials",
					  nanogui::VariableType::Float32,
					  { numMaterials, sizeof(MaterialCPU) / sizeof(float) },
					  materialsCPU.data()
					  );
	
	size_t dummy_texture_count = shader.get_buffer_size("textures");
	
	for (size_t i = textureSetCount; i < dummy_texture_count; ++i) {
		std::string textureBaseName = "textures";
		shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
	}
}

MeshBatch::MeshBatch(nanogui::RenderPass& renderPass) : mRenderPass(renderPass) {
}

void MeshBatch::add_mesh(std::reference_wrapper<Mesh> mesh) {
	int instanceId = mesh.get().get_metadata_component().identifier();
	
	auto instanceIt = mMeshes.find(instanceId);
	
	if (instanceIt == mMeshes.end()) {
		append(mesh);
	}
	
	mMeshes[instanceId].push_back(mesh);
}

void MeshBatch::clear() {
	mMeshes.clear();
	mBatchPositions.clear();
	mBatchNormals.clear();
	mBatchTexCoords1.clear();
	mBatchTexCoords2.clear();
	mBatchMaterialIds.clear();
	mBatchIndices.clear();
	mBatchMaterials.clear();
	mMeshStartIndices.clear();
	mMeshVertexStartIndices.clear();
	mVertexIndexingMap.clear();
}

void MeshBatch::append(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	
	int instanceId = mesh.get_metadata_component().identifier();
		
	auto& shader = mesh.get_shader();
	int identifier = shader.identifier();
		
	auto& indexer = mVertexIndexingMap[instanceId];
	
	size_t vertexOffset = indexer.mVertexOffset;
	size_t indexOffset = indexer.mIndexOffset;
	
	mMeshStartIndices[instanceId].push_back(indexOffset);
	mMeshVertexStartIndices[instanceId].push_back(vertexOffset);
	
	// Append vertex data using getters
	mBatchPositions[identifier].insert(mBatchPositions[identifier].end(),
									   mesh.get_flattened_positions().begin(),
									   mesh.get_flattened_positions().end());
	mBatchNormals[identifier].insert(mBatchNormals[identifier].end(),
									 mesh.get_flattened_normals().begin(),
									 mesh.get_flattened_normals().end());
	mBatchTexCoords1[identifier].insert(mBatchTexCoords1[identifier].end(),
										mesh.get_flattened_tex_coords1().begin(),
										mesh.get_flattened_tex_coords1().end());
	mBatchTexCoords2[identifier].insert(mBatchTexCoords2[identifier].end(),
										mesh.get_flattened_tex_coords2().begin(),
										mesh.get_flattened_tex_coords2().end());
	mBatchMaterialIds[identifier].insert(mBatchMaterialIds[identifier].end(),
										 mesh.get_flattened_material_ids().begin(),
										 mesh.get_flattened_material_ids().end());
	mBatchColors[identifier].insert(mBatchColors[identifier].end(),
									mesh.get_flattened_colors().begin(),
									mesh.get_flattened_colors().end());
	
	// Adjust and append indices
	auto& indices = mesh.get_mesh_data().get_indices();
	for (auto index : indices) {
		mBatchIndices[identifier].push_back(index + vertexOffset);
	}
	
	indexer.mIndexOffset += indices.size();
	indexer.mVertexOffset += mesh.get_mesh_data().get_vertices().size();
	
	upload_vertex_data(shader, identifier);
}

void MeshBatch::remove(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	auto& shader = mesh.get_shader();
	int identifier = shader.identifier();
	
	auto instanceId = mesh.get_metadata_component().identifier();
	int entityId = mesh.get_color_component().identifier();
	
	auto mesh_it = mMeshes.find(instanceId);
	
	if (mesh_it == mMeshes.end()) {
		// instance not found
		return;
	}
	
	auto& mesh_vector = mesh_it->second;
	
	// Find the mesh in the vector
	auto mesh_ptr = &mesh;
	auto it = std::find_if(mesh_vector.begin(), mesh_vector.end(),
						   [mesh_ptr](const std::reference_wrapper<Mesh>& m) {
		return &m.get() == mesh_ptr;
	});
	
	if (it == mesh_vector.end()) {
		// Mesh not found in the vector; nothing to remove
		return;
	}
	
	// Determine the index of the mesh within the vector
	size_t meshIndex = std::distance(mesh_vector.begin(), it);
	
	// Remove the mesh from the mesh vector
	mesh_vector.erase(it);
	
	if (mesh_vector.empty()) {
		
		// Get starting indices
		size_t indexStartIdx = mMeshStartIndices[instanceId][meshIndex];
		size_t indexCount = mesh.get_mesh_data().get_indices().size();
		
		size_t vertexStartIdx = mMeshVertexStartIndices[instanceId][meshIndex];
		size_t vertexCount = mesh.get_mesh_data().get_vertices().size();
		size_t vertexEndIdx = vertexStartIdx + vertexCount;

		// Remove indices
		mBatchIndices[identifier].erase(mBatchIndices[identifier].begin() + indexStartIdx,
										mBatchIndices[identifier].begin() + indexStartIdx + vertexCount);
		
		// Adjust indices after the removed indices
		for (size_t i = 0; i < mBatchIndices[identifier].size(); ++i) {
			if (mBatchIndices[identifier][i] >= vertexEndIdx) {
				mBatchIndices[identifier][i] -= vertexCount;
			} else if (mBatchIndices[identifier][i] >= vertexStartIdx) {
				mBatchIndices[identifier][i] = 0; // Or handle appropriately if needed
			}
		}
		
		// Remove per-vertex data
		auto removeRange = [](auto& dataVec, size_t startIdx, size_t count, size_t componentsPerVertex) {
			dataVec.erase(
						  dataVec.begin() + startIdx * componentsPerVertex,
						  dataVec.begin() + (startIdx + count) * componentsPerVertex
						  );
		};
		
		removeRange(mBatchPositions[identifier], vertexStartIdx, vertexCount, 3);
		removeRange(mBatchNormals[identifier], vertexStartIdx, vertexCount, 3);
		removeRange(mBatchTexCoords1[identifier], vertexStartIdx, vertexCount, 2);
		removeRange(mBatchTexCoords2[identifier], vertexStartIdx, vertexCount, 2);
		removeRange(mBatchMaterialIds[identifier], vertexStartIdx, vertexCount, 1);
		removeRange(mBatchColors[identifier], vertexStartIdx, vertexCount, 4);
		
		// Remove from mMeshStartIndices and mMeshVertexStartIndices
		mMeshStartIndices[instanceId].erase(mMeshStartIndices[instanceId].begin() + meshIndex);
		mMeshVertexStartIndices[instanceId].erase(mMeshVertexStartIndices[instanceId].begin() + meshIndex);
		
		// Adjust subsequent start indices
		for (size_t i = meshIndex; i < mMeshStartIndices[instanceId].size(); ++i) {
			mMeshStartIndices[instanceId][i] -= indexCount;
			mMeshVertexStartIndices[instanceId][i] -= vertexCount;
		}
		
		// Update indexer
		auto& indexer = mVertexIndexingMap[instanceId];
		indexer.mIndexOffset -= indexCount;
		indexer.mVertexOffset -= vertexCount;
		
		mMeshes.erase(mesh_it);
	} else {
		// Re-upload updated vertex and index data to the GPU
		upload_vertex_data(shader, identifier);
	}
}

void MeshBatch::upload_vertex_data(ShaderWrapper& shader, int identifier) {
	// Upload consolidated data to GPU
	shader.persist_buffer("aPosition", nanogui::VariableType::Float32, { mBatchPositions[identifier].size() / 3, 3 },
						  mBatchPositions[identifier].data());
	shader.persist_buffer("aNormal", nanogui::VariableType::Float32, { mBatchNormals[identifier].size() / 3, 3 },
						  mBatchNormals[identifier].data());
	shader.persist_buffer("aTexcoords1", nanogui::VariableType::Float32, { mBatchTexCoords1[identifier].size() / 2, 2 },
						  mBatchTexCoords1[identifier].data());
	shader.persist_buffer("aTexcoords2", nanogui::VariableType::Float32, { mBatchTexCoords2[identifier].size() / 2, 2 },
						  mBatchTexCoords2[identifier].data());
	shader.persist_buffer("aMaterialId", nanogui::VariableType::Int32, { mBatchMaterialIds[identifier].size(), 1 },
						  mBatchMaterialIds[identifier].data());
	shader.persist_buffer("aColor", nanogui::VariableType::Float32, { mBatchColors[identifier].size() / 4, 4 },
						  mBatchColors[identifier].data());
	
	// Upload indices
	shader.persist_buffer("indices", nanogui::VariableType::UInt32, { mBatchIndices[identifier].size() },
						  mBatchIndices[identifier].data());
}

void MeshBatch::draw_content(const nanogui::Matrix4f& view,
							 const nanogui::Matrix4f& projection) {
	for (auto& [instance_id, mesh_vector] : mMeshes) {
		
		if (mesh_vector.empty()) continue;
		
		for (size_t i = 0; i < mesh_vector.size(); ++i) {
			auto& mesh = mesh_vector[i].get();
			
			if (!mesh.get_color_component().get_visible()) {
				continue;
			}
			
			
			auto& shader = mesh.get_shader();
			shader.set_uniform("aProjection", projection);
			shader.set_uniform("aView", view);
			// Set the model matrix for the current mesh
			shader.set_uniform("aModel", mesh.get_model_matrix());
			
			int entityId = mesh.get_color_component().identifier();

			// Apply color component (assuming it sets relevant uniforms)
			
			shader.set_uniform("identifier", mesh.get_color_component().identifier());
			
			shader.set_uniform("color", glm_to_nanogui(mesh.get_color_component().get_color()));
			
			// Upload materials for the current mesh
			upload_material_data(shader, mesh.get_mesh_data().get_material_properties());

			int shader_id = shader.identifier();

			// Calculate the range of indices to draw for this mesh
			size_t startIdx = mMeshStartIndices[instance_id][0];
			
			// By default, assume the end index is the size of the batch indices
			size_t count = mesh.get_mesh_data().get_indices().size();
			
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
