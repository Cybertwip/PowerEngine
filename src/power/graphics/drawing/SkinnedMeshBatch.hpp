#pragma once


class SkinnedMeshBatch {
private:
	struct VertexIndexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};
	
public:
	SkinnedMeshBatch(nanogui::RenderPass& renderPass);
	
	void add_mesh(std::reference_wrapper<SkinnedMesh> mesh);
	void clear();
	void append(std::reference_wrapper<SkinnedMesh> meshRef);
	void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection);
	
private:
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	
	std::unordered_map<ShaderWrapper*, std::vector<std::reference_wrapper<SkinnedMesh>>> mMeshes;
	
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
