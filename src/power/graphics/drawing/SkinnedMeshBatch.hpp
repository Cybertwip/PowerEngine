#pragma once

#include "graphics/drawing/ISkinnedMeshBatch.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <nanogui/vector.h>

#include <functional>

namespace nanogui {
class RenderPass;
}

class ShaderWrapper;
class SkinnedMesh;

class SkinnedMeshBatch : public ISkinnedMeshBatch {
private:
	struct VertexIndexer {
		unsigned int mVertexOffset = 0;
		unsigned int mIndexOffset = 0;
	};
	
public:
	SkinnedMeshBatch(nanogui::RenderPass& renderPass);
	
	void add_mesh(std::reference_wrapper<SkinnedMesh> mesh) override;
	void clear() override;
	
	void remove(std::reference_wrapper<SkinnedMesh> mesh) override;

	void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
	const std::unordered_map<int, std::vector<float>>& get_batch_positions() {
		return mBatchPositions;;
	}

private:
	void append(std::reference_wrapper<SkinnedMesh> meshRef) override;

	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	
	void upload_vertex_data(ShaderWrapper& shader, int identifier);
	
	std::unordered_map<int, std::vector<std::reference_wrapper<SkinnedMesh>>> mMeshes;
	
	// Consolidated buffers
	std::unordered_map<int, std::vector<float>> mBatchPositions;
	std::unordered_map<int, std::vector<float>> mBatchNormals;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords1;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords2;
	std::unordered_map<int, std::vector<int>> mBatchMaterialIds;
	std::unordered_map<int, std::vector<int>> mBatchBoneIds;
	std::unordered_map<int, std::vector<float>> mBatchBoneWeights;
	std::unordered_map<int, std::vector<unsigned int>> mBatchIndices;
	std::unordered_map<int, std::vector<float>> mBatchColors;
	std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
	
	std::map<int, std::vector<size_t>> mMeshStartIndices;

	std::unordered_map<int, std::vector<size_t>> mMeshVertexStartIndices;

	std::unordered_map<int, VertexIndexer> mVertexIndexingMap;

	nanogui::RenderPass& mRenderPass;
};
