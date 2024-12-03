// MeshBatch.hpp
#pragma once

#include "graphics/drawing/IMeshBatch.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include <nanogui/vector.h>
#include <functional>

namespace nanogui {
class RenderPass;
}

class Mesh;
class ShaderWrapper;

class MeshBatch : public IMeshBatch {
private:
	static constexpr size_t MAX_BATCH_SIZE = 1000000; // Configurable batch size
	
	struct BatchData {
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texCoords1;
		std::vector<float> texCoords2;
		std::vector<int> materialIds;
		std::vector<float> colors;
		std::vector<unsigned int> indices;
		std::vector<std::shared_ptr<MaterialProperties>> materials;
		size_t vertexOffset = 0;
		size_t indexOffset = 0;
	};
	
public:
	MeshBatch(nanogui::RenderPass& renderPass);
	~MeshBatch() = default;
	
	void add_mesh(std::reference_wrapper<Mesh> mesh) override;
	void clear() override;
	void remove(std::reference_wrapper<Mesh> meshRef) override;
	void draw_content(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
private:
	void append(std::reference_wrapper<Mesh> meshRef) override;
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	void upload_vertex_data(ShaderWrapper& shader, int identifier, size_t batchIndex);
	size_t find_or_create_batch(int identifier, size_t requiredVertices);
	
	// Main data structures
	std::unordered_map<int, std::vector<std::reference_wrapper<Mesh>>> mMeshes;
	std::unordered_map<int, std::vector<BatchData>> mBatches; // shader ID -> batches
	
	// Mesh tracking
	std::unordered_map<int, std::unordered_map<int, size_t>> mMeshBatchIndex; // instance ID -> batch index
	std::unordered_map<int, std::unordered_map<int, size_t>> mMeshOffsetInBatch; // instance ID -> offset in batch
	std::unordered_map<int, std::unordered_map<int, size_t>> mMeshIndexCount; // instance ID -> index count
	
	nanogui::RenderPass& mRenderPass;
};
