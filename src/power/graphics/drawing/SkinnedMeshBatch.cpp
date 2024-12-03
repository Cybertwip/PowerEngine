#include "SkinnedMeshBatch.hpp"

#include "components/ColorComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <glm/gtc/type_ptr.hpp>


namespace SkinnedMeshBatchUtils {
struct BoneCPU {
	float transform[4][4] =
	{
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f }
	};
};

// Retrieve the bones for rendering
std::vector<BoneCPU> build_cpu_bones(SkeletonComponent& skeletonComponent) {
	// Ensure we have a valid number of bones
	size_t numBones = skeletonComponent.get_skeleton().num_bones();
	
	std::vector<BoneCPU> bonesCPU(numBones);
	
	for (size_t i = 0; i < numBones; ++i) {
		Skeleton::Bone& bone = static_cast<Skeleton::Bone&>(skeletonComponent.get_skeleton().get_bone(i));
		
		// Get the bone transform as a glm::mat4
		glm::mat4 boneTransform = bone.global;
		
		// Reference to the BoneCPU structure
		BoneCPU& boneCPU = bonesCPU[i];
		
		// Copy each element from glm::mat4 to the BoneCPU's transform array
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				boneCPU.transform[row][col] = boneTransform[row][col];
			}
		}
	}
	return bonesCPU;
}
}
SkinnedMeshBatch::SkinnedMeshBatch(nanogui::RenderPass& renderPass) : mRenderPass(renderPass) {
}

void SkinnedMeshBatch::add_mesh(std::reference_wrapper<SkinnedMesh> mesh) {
	int instanceId = mesh.get().get_metadata_component().identifier();
	
	auto instanceIt = mMeshes.find(instanceId);
	if (instanceIt == mMeshes.end()) {
		append(mesh);
	}
	
	mMeshes[instanceId].push_back(mesh);
}

void SkinnedMeshBatch::clear() {
	mMeshes.clear();
	mBatches.clear();
	mMeshBatchIndex.clear();
	mMeshOffsetInBatch.clear();
	mMeshIndexCount.clear();
}

size_t SkinnedMeshBatch::find_or_create_batch(int identifier, size_t requiredVertices) {
	auto& batchList = mBatches[identifier];
	for (size_t i = 0; i < batchList.size(); i++) {
		if (batchList[i].vertexOffset + requiredVertices <= MAX_BATCH_SIZE) {
			return i;
		}
	}
	batchList.push_back(BatchData());
	return batchList.size() - 1;
}

void SkinnedMeshBatch::append(std::reference_wrapper<SkinnedMesh> meshRef) {
	auto& mesh = meshRef.get();
	int instanceId = mesh.get_metadata_component().identifier();
	auto& shader = mesh.get_shader();
	int identifier = shader.identifier();
	
	size_t vertexCount = mesh.get_mesh_data().get_vertices().size();
	size_t batchIndex = find_or_create_batch(identifier, vertexCount);
	BatchData& batch = mBatches[identifier][batchIndex];
	
	// Store mesh information
	mMeshBatchIndex[instanceId][identifier] = batchIndex;
	mMeshOffsetInBatch[instanceId][identifier] = batch.vertexOffset;
	mMeshIndexCount[instanceId][identifier] = mesh.get_mesh_data().get_indices().size();
	
	// Append vertex data
	batch.positions.insert(batch.positions.end(),
						   mesh.get_flattened_positions().begin(),
						   mesh.get_flattened_positions().end());
	// [Similar insertions for other attributes]
	batch.boneIds.insert(batch.boneIds.end(),
						 mesh.get_flattened_bone_ids().begin(),
						 mesh.get_flattened_bone_ids().end());
	batch.boneWeights.insert(batch.boneWeights.end(),
							 mesh.get_flattened_weights().begin(),
							 mesh.get_flattened_weights().end());
	
	// Append and adjust indices
	for (auto index : mesh.get_mesh_data().get_indices()) {
		batch.indices.push_back(index + batch.vertexOffset);
	}
	
	// Update batch offsets
	batch.vertexOffset += vertexCount;
	batch.indexOffset += mesh.get_mesh_data().get_indices().size();
	
	// Upload the current batch
	upload_vertex_data(shader, identifier, batchIndex);
}

void SkinnedMeshBatch::remove(std::reference_wrapper<SkinnedMesh> meshRef) {
	auto& mesh = meshRef.get();
	int instanceId = mesh.get_metadata_component().identifier();
	int identifier = mesh.get_shader().identifier();
	
	auto meshIt = mMeshes.find(instanceId);
	if (meshIt == mMeshes.end()) return;
	
	auto& meshVector = meshIt->second;
	auto it = std::find_if(meshVector.begin(), meshVector.end(),
						   [&mesh](const auto& m) { return &m.get() == &mesh; });
	
	if (it == meshVector.end()) return;
	
	meshVector.erase(it);
	
	if (meshVector.empty()) {
		size_t batchIndex = mMeshBatchIndex[instanceId][identifier];
		auto& batch = mBatches[identifier][batchIndex];
		batch = BatchData();
		
		// Rebuild batch from remaining meshes
		for (const auto& [id, meshes] : mMeshes) {
			for (const auto& m : meshes) {
				if (m.get().get_shader().identifier() == identifier) {
					append(m);
				}
			}
		}
		
		mMeshes.erase(meshIt);
		mMeshBatchIndex[instanceId].erase(identifier);
		mMeshOffsetInBatch[instanceId].erase(identifier);
		mMeshIndexCount[instanceId].erase(identifier);
	}
}

// [upload_material_data remains largely the same]

void SkinnedMeshBatch::upload_vertex_data(ShaderWrapper& shader, int identifier, size_t batchIndex) {
	const auto& batch = mBatches[identifier][batchIndex];
	
	shader.persist_buffer("aPosition", nanogui::VariableType::Float32,
						  {batch.positions.size() / 3, 3}, batch.positions.data());
	// [Similar buffer uploads for other attributes]
	shader.persist_buffer("aBoneIds", nanogui::VariableType::Int32,
						  {batch.boneIds.size() / 4, 4}, batch.boneIds.data());
	shader.persist_buffer("aWeights", nanogui::VariableType::Float32,
						  {batch.boneWeights.size() / 4, 4}, batch.boneWeights.data());
	shader.persist_buffer("indices", nanogui::VariableType::UInt32,
						  {batch.indices.size()}, batch.indices.data());
}

void SkinnedMeshBatch::draw_content(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	for (const auto& [identifier, batches] : mBatches) {
		for (size_t batchIndex = 0; batchIndex < batches.size(); ++batchIndex) {
			for (const auto& [instanceId, meshVector] : mMeshes) {
				for (const auto& meshRef : meshVector) {
					auto& mesh = meshRef.get();
					
					if (!mesh.get_color_component().get_visible() ||
						mesh.get_shader().identifier() != identifier ||
						mMeshBatchIndex[instanceId][identifier] != batchIndex) {
						continue;
					}
					
					auto& shader = mesh.get_shader();
					// [Set uniforms and draw as before]
					auto bones = SkinnedMeshBatchUtils::build_cpu_bones(mesh.get_skeleton_component());
					shader.set_buffer("bones", nanogui::VariableType::Float32,
									  {bones.size(), sizeof(SkinnedMeshBatchUtils::BoneCPU) / sizeof(float)},
									  bones.data());
					
					size_t startIdx = mMeshOffsetInBatch[instanceId][identifier];
					size_t count = mMeshIndexCount[instanceId][identifier];
					
					shader.begin();
					shader.draw_array(nanogui::Shader::PrimitiveType::Triangle, startIdx, count, true);
					shader.end();
				}
			}
		}
	}
}
