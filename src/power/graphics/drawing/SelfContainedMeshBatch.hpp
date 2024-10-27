#pragma once

#include "graphics/drawing/IMeshBatch.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include <nanogui/vector.h>
#include <functional>
#include <vector>
#include <unordered_map>

namespace nanogui {
class RenderPass;
}

class Mesh;
class ShaderWrapper;

class SelfContainedMeshBatch : public IMeshBatch {
public:
	struct MaterialCPU {
		float mAmbient[4];
		float mDiffuse[4];
		float mSpecular[4];
		float mShininess;
		float mOpacity;
		float mHasDiffuseTexture;
		float _;
	};
	
	struct Indexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};
	
	SelfContainedMeshBatch(nanogui::RenderPass& renderPass, ShaderWrapper& shader);
	
	void add_mesh(std::reference_wrapper<Mesh> mesh) override;
	void clear() override;
	void append(std::reference_wrapper<Mesh> meshRef) override;
	void remove(std::reference_wrapper<Mesh> meshRef) override;
	void draw_content(const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
private:
	void upload_material_data(const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	void upload_vertex_data();
	
	nanogui::RenderPass& mRenderPass;
	ShaderWrapper& mShader;
	
	std::vector<std::reference_wrapper<Mesh>> mMeshes;
	
	// Consolidated buffers
	std::vector<float> mBatchPositions;
	std::vector<float> mBatchNormals;
	std::vector<float> mBatchTexCoords1;
	std::vector<float> mBatchTexCoords2;
	std::vector<int> mBatchMaterialIds;
	std::vector<unsigned int> mBatchIndices;
	std::vector<float> mBatchColors;
	
	// Offset tracking
	std::vector<size_t> mMeshStartIndices;
	std::vector<size_t> mMeshVertexStartIndices;
	
	Indexer mIndexer;
};
