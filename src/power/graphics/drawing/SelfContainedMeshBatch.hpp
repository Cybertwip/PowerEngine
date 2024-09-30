#pragma once

#include "graphics/drawing/Batch.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <nanogui/vector.h>

#include <functional>

namespace nanogui {
class RenderPass;
}

class Mesh;
class ShaderWrapper;

class SelfContainedMeshBatch : public Batch {
private:
	struct VertexIndexer {
		unsigned int mVertexOffset = 0;
		unsigned int mIndexOffset = 0;
	};
	
public:
	SelfContainedMeshBatch(nanogui::RenderPass& renderPass, ShaderWrapper& shader);
	
	void add_mesh(std::reference_wrapper<Mesh> mesh);
	void clear();
	void append(std::reference_wrapper<Mesh> meshRef);
	void remove(std::reference_wrapper<Mesh> meshRef);
	void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
	const std::unordered_map<int, std::vector<float>>& get_batch_positions() {
		return mBatchPositions;;
	}
	
private:
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	
	void upload_vertex_data(ShaderWrapper& shader, int identifier);

	std::unordered_map<ShaderWrapper*, std::vector<std::reference_wrapper<Mesh>>> mMeshes;
	
	// Consolidated buffers
	std::unordered_map<int, std::vector<float>> mBatchPositions;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords1;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords2;
	std::unordered_map<int, std::vector<int>> mBatchMaterialIds;
	std::unordered_map<int, std::vector<unsigned int>> mBatchIndices;
	std::unordered_map<int, std::vector<float>> mBatchColors;
	std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
	
	// Offset tracking
	std::unordered_map<int, std::vector<size_t>> mMeshStartIndices;
	
	std::unordered_map<int, VertexIndexer> mVertexIndexingMap;
	
	nanogui::RenderPass& mRenderPass;
	ShaderWrapper& mShader;
};
