#include "SelfContainedMeshBatch.hpp"

#include "components/ColorComponent.hpp"

#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <glm/gtc/type_ptr.hpp>

void SelfContainedMeshBatch::upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
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
			shader.set_texture(textureBaseName, *material.mTextureDiffuse, i);
			
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
					   {numMaterials, sizeof(MaterialCPU) / sizeof(float)},
					   materialsCPU.data()
					   );

	size_t dummy_texture_count = shader.get_buffer_size("textures");
	
	for (size_t i = textureSetCount; i<dummy_texture_count; ++i) {
		std::string textureBaseName = "textures";

		shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
	}
}


SelfContainedMeshBatch::SelfContainedMeshBatch(nanogui::RenderPass& renderPass, ShaderWrapper& shader) : mRenderPass(renderPass), mShader(shader) {
}

void SelfContainedMeshBatch::add_mesh(std::reference_wrapper<Mesh> mesh) {
	
	auto it = std::find_if(mMeshes.begin(), mMeshes.end(), [mesh](auto kp) {
		return kp.first->identifier() == mesh.get().get_shader().identifier();
	});

	if (it != mMeshes.end()) {
		it->second.push_back(mesh);
	} else {
		mMeshes[&(mShader)].push_back(mesh);
	}
}

void SelfContainedMeshBatch::clear() {
	mBatchPositions.clear();
	mBatchNormals.clear();
	mBatchTexCoords1.clear();
	mBatchTexCoords2.clear();
	mBatchMaterialIds.clear();
	mBatchIndices.clear();
	mBatchMaterials.clear();
	mMeshStartIndices.clear();
}

void SelfContainedMeshBatch::append(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	
	auto& shader = mShader;
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
	mBatchMaterialIds[shader.identifier()].insert(mBatchMaterialIds[shader.identifier()].end(),
												 mesh.get_flattened_material_ids().begin(),
												 mesh.get_flattened_material_ids().end());
	
	mBatchColors[shader.identifier()].insert(mBatchColors[shader.identifier()].end(),
											 mesh.get_flattened_colors().begin(),
											 mesh.get_flattened_colors().end());
	
	// Adjust and append indices
	auto& indices = mesh.get_mesh_data().get_indices()
	;
	for (auto index : indices) {
		mBatchIndices[shader.identifier()].push_back(index + indexer.mVertexOffset);
	}
	
	mMeshStartIndices[shader.identifier()].push_back(indexer.mIndexOffset);
	
	indexer.mIndexOffset += mesh.get_mesh_data().get_indices().size();
	indexer.mVertexOffset += mesh.get_mesh_data().get_vertices().size();
	
	upload_vertex_data(shader, identifier);
}


void SelfContainedMeshBatch::upload_vertex_data(ShaderWrapper& shader, int identifier) {
	
	// Upload consolidated data to GPU
	shader.persist_buffer("aPosition", nanogui::VariableType::Float32, {mBatchPositions[identifier].size() / 3, 3},
					  mBatchPositions[identifier].data());
	shader.persist_buffer("aNormal", nanogui::VariableType::Float32, {mBatchNormals[identifier].size() / 3, 3},
					  mBatchNormals[identifier].data());
	shader.persist_buffer("aTexcoords1", nanogui::VariableType::Float32, {mBatchTexCoords1[identifier].size() / 2, 2},
					  mBatchTexCoords1[identifier].data());
	shader.persist_buffer("aTexcoords2", nanogui::VariableType::Float32, {mBatchTexCoords2[identifier].size() / 2, 2},
					  mBatchTexCoords2[identifier].data());
	shader.persist_buffer("aMaterialId", nanogui::VariableType::Int32, {mBatchMaterialIds[identifier].size(), 1},
						  mBatchMaterialIds[identifier].data());

	// Set Buffer for Vertex Colors
	shader.persist_buffer("aColor", nanogui::VariableType::Float32, {mBatchColors[identifier].size() / 4, 4},
					  mBatchColors[identifier].data());
	// Upload indices
	shader.persist_buffer("indices", nanogui::VariableType::UInt32, {mBatchIndices[identifier].size()},
					  mBatchIndices[identifier].data());
}

void SelfContainedMeshBatch::remove(std::reference_wrapper<Mesh> meshRef) {
	auto& mesh = meshRef.get();
	auto& shader = mShader;
	int identifier = shader.identifier();
	
	// 1. Locate the mesh in mMeshes
	auto mesh_it = mMeshes.find(&shader);
	if (mesh_it == mMeshes.end()) {
		// Shader not found; nothing to remove
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
	
	// 2. Remove the mesh from the mesh vector
	mesh_vector.erase(it);
	
	// 3. Remove associated vertex and index data
	// Ensure that mMeshStartIndices has an entry for this shader
	auto startIdxIt = mMeshStartIndices.find(identifier);
	if (startIdxIt == mMeshStartIndices.end()) {
		// No start indices found; inconsistent state
		assert(false && "Start indices for shader identifier not found.");
		return;
	}
	
	auto& meshStartIndices = startIdxIt->second;
	
	if (meshIndex >= meshStartIndices.size()) {
		// Invalid mesh index; inconsistent state
		assert(false && "Mesh index out of range in start indices.");
		return;
	}
	
	size_t startIdx = meshStartIndices[meshIndex];
	size_t endIdx;
	
	if (meshIndex + 1 < meshStartIndices.size()) {
		endIdx = meshStartIndices[meshIndex + 1];
	} else {
		endIdx = mBatchIndices[identifier].size();
	}
	
	size_t count = endIdx - startIdx;
	
	// 3.a. Remove indices
	mBatchIndices[identifier].erase(
									mBatchIndices[identifier].begin() + startIdx,
									mBatchIndices[identifier].begin() + endIdx
									);
	
	// 3.b. Remove vertex data
	// Assuming that each mesh's vertices are contiguous and added sequentially
	// Calculate the number of vertices to remove
	size_t numVertices = mesh.get_mesh_data().get_vertices().size();
	size_t vertexCount = numVertices * 3; // Assuming 3 floats per vertex position
	
	// Remove from positions
	auto& positions = mBatchPositions[identifier];
	positions.erase(
					positions.begin() + startIdx * 3, // 3 floats per position
					positions.begin() + (startIdx + numVertices) * 3
					);
	
	// Remove from normals
	auto& normals = mBatchNormals[identifier];
	normals.erase(
				  normals.begin() + startIdx * 3,
				  normals.begin() + (startIdx + numVertices) * 3
				  );
	
	// Remove from texcoords1
	auto& texcoords1 = mBatchTexCoords1[identifier];
	texcoords1.erase(
					 texcoords1.begin() + startIdx * 2, // 2 floats per texcoord
					 texcoords1.begin() + (startIdx + numVertices) * 2
					 );
	
	// Remove from texcoords2
	auto& texcoords2 = mBatchTexCoords2[identifier];
	texcoords2.erase(
					 texcoords2.begin() + startIdx * 2,
					 texcoords2.begin() + (startIdx + numVertices) * 2
					 );
	
	// Remove from texture IDs
	auto& materialIds = mBatchMaterialIds[identifier];
	materialIds.erase(
					  materialIds.begin() + startIdx,
					  materialIds.begin() + (startIdx + numVertices)
					 );
	
	// Remove from colors
	auto& colors = mBatchColors[identifier];
	colors.erase(
				 colors.begin() + startIdx * 4, // 4 floats per color
				 colors.begin() + (startIdx + numVertices) * 4
				 );
		
	// 3.c. Update mesh start indices
	meshStartIndices.erase(meshStartIndices.begin() + meshIndex);
	
	// Adjust subsequent start indices
	for (size_t i = meshIndex; i < meshStartIndices.size(); ++i) {
		meshStartIndices[i] -= count;
	}
	
	// 3.d. Update vertex indexing map
	auto& indexer = mVertexIndexingMap[identifier];
	indexer.mVertexOffset -= numVertices;
	indexer.mIndexOffset -= count;
	
	// If there are no more meshes using this shader, clean up
	if (mesh_vector.empty()) {
		mMeshes.erase(&shader);
		mBatchPositions.erase(identifier);
		mBatchNormals.erase(identifier);
		mBatchTexCoords1.erase(identifier);
		mBatchTexCoords2.erase(identifier);
		mBatchMaterialIds.erase(identifier);
		mBatchIndices.erase(identifier);
		mBatchColors.erase(identifier);
		mMeshStartIndices.erase(identifier);
		mVertexIndexingMap.erase(identifier);
	} else {
		
		// 4. Re-upload updated vertex and index data to the GPU
		// Note: Depending on your rendering backend, you might need to handle this differently
		// For simplicity, we'll re-upload all data for this shader
		upload_vertex_data(shader, identifier);
		
	}
}



void SelfContainedMeshBatch::draw_content(const nanogui::Matrix4f& view,
										  const nanogui::Matrix4f& projection) {
	for (auto& [shader_pointer, mesh_vector] : mMeshes) {
		auto& shader = *shader_pointer;
		int identifier = shader.identifier();
				
		mRenderPass.pop_depth_test_state(identifier);
		
		for (int i = 0; i<mesh_vector.size(); ++i) {
			auto& mesh = mesh_vector[i].get();
//			
			if (!mesh.get_color_component().get_visible()) {
				continue;
			}
						
			// Set uniforms and draw the mesh content
			shader.set_uniform("aProjection", projection);

			shader.set_uniform("aView", view);

			// Set the model matrix for the current mesh
			shader.set_uniform("aModel", mesh.get_model_matrix());

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