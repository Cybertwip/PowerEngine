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

class MeshBatch : public Batch {
private:
	struct VertexIndexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};
	
public:
	MeshBatch(nanogui::RenderPass& renderPass);
	
	void add_mesh(std::reference_wrapper<Mesh> mesh);
	void clear();
	void append(std::reference_wrapper<Mesh> meshRef);
	void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
private:
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	
	std::unordered_map<ShaderWrapper*, std::vector<std::reference_wrapper<Mesh>>> mMeshes;
	
	// Consolidated buffers
	std::unordered_map<int, std::vector<float>> mBatchPositions;
	std::unordered_map<int, std::vector<float>> mBatchNormals;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords1;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords2;
	std::unordered_map<int, std::vector<int>> mBatchTextureIds;
	std::unordered_map<int, std::vector<unsigned int>> mBatchIndices;
	std::unordered_map<int, std::vector<float>> mBatchColors;
	std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
	
	// Offset tracking
	std::unordered_map<int, std::vector<size_t>> mMeshStartIndices;
	
	std::unordered_map<int, VertexIndexer> mVertexIndexingMap;
	
	nanogui::RenderPass& mRenderPass;
};
