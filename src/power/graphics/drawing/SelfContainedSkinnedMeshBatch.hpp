#pragma once

#include "components/ColorComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/renderpass.h>

#include <vector>
#include <unordered_map>
#include <functional>

class SelfContainedSkinnedMeshBatch {
public:
	struct MaterialCPU {
		float mAmbient[4];
		float mDiffuse[4];
		float mSpecular[4];
		float mShininess;
		float mOpacity;
		float mHasDiffuseTexture;
		// Padding to align to 16 bytes if necessary
	};
	
	struct Indexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};
	
	SelfContainedSkinnedMeshBatch(nanogui::RenderPass& renderPass, ShaderWrapper& shader);
	
	void add_mesh(std::reference_wrapper<SkinnedMesh> mesh);
	void remove(std::reference_wrapper<SkinnedMesh> mesh);
	void clear();
	void draw_content(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection);
	
	const std::vector<float>& get_batch_positions() {
		return mBatchPositions;
	}
	
private:
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	void append(std::reference_wrapper<SkinnedMesh> meshRef);
	void upload_vertex_data();
	
	nanogui::RenderPass& mRenderPass;
	ShaderWrapper& mShader;
	
	std::vector<std::reference_wrapper<SkinnedMesh>> mMeshes;
	
	std::vector<float> mBatchPositions;
	std::vector<float> mBatchNormals;
	std::vector<float> mBatchTexCoords1;
	std::vector<float> mBatchTexCoords2;
	std::vector<int> mBatchMaterialIds;
	std::vector<float> mBatchColors;
	std::vector<int> mBatchBoneIds;
	std::vector<float> mBatchBoneWeights;
	std::vector<uint32_t> mBatchIndices;
	
	std::vector<size_t> mMeshStartIndices;
	std::vector<size_t> mMeshVertexStartIndices;
	
	Indexer mIndexer;
};
