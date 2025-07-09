#pragma once

#include "components/TransformAnimationComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>

class ColorComponent;
class SkeletonComponent;
class SkinnedAnimationComponent;
class ISkinnedMeshBatch;
class ShaderManager;

class SkinnedMesh : public Drawable {
public:
	SkinnedMesh(MeshData& skinnedMeshData, ShaderWrapper& shader, ISkinnedMeshBatch& meshBatch, MetadataComponent& metadataComponent, ColorComponent& colorComponent,
		SkeletonComponent& skeletonComponent);
	~SkinnedMesh() override;
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	ShaderWrapper& get_shader() const { return mShader; }
	
	// Getters for flattened data
	const std::vector<float>& get_flattened_positions() const { return mFlattenedPositions; }
	const std::vector<float>& get_flattened_normals() const { return mFlattenedNormals; }
	const std::vector<float>& get_flattened_tex_coords1() const { return mFlattenedTexCoords1; }
	const std::vector<float>& get_flattened_tex_coords2() const { return mFlattenedTexCoords2; }
	const std::vector<int>& get_flattened_bone_ids() const { return mFlattenedBoneIds; }
	const std::vector<float>& get_flattened_weights() const { return mFlattenedWeights; }
	const std::vector<int>& get_flattened_material_ids() const { return mFlattenedMaterialIds; }
	const std::vector<float>& get_flattened_colors() const { return mFlattenedColors; }
	
	SkinnedMeshData& get_mesh_data() {
		return static_cast<SkinnedMeshData&>(mMeshData);
	}
	
	MetadataComponent& get_metadata_component() const {
		return mMetadataComponent;
	}

	ColorComponent& get_color_component() const {
		return mColorComponent;
	}

	SkeletonComponent& get_skeleton_component() const {
		return mSkeletonComponent;
	}

	const nanogui::Matrix4f& get_model_matrix() {
		return mModelMatrix;
	}
	
private:
	MeshData& mMeshData;
	
	ShaderWrapper& mShader;
	
	// Flattened data
	std::vector<float> mFlattenedPositions;
	std::vector<float> mFlattenedNormals;
	std::vector<float> mFlattenedTexCoords1;
	std::vector<float> mFlattenedTexCoords2;
	std::vector<int> mFlattenedBoneIds;
	std::vector<float> mFlattenedWeights;
	std::vector<int> mFlattenedMaterialIds;
	std::vector<float> mFlattenedColors; // Added to store flattened color data
	
	ISkinnedMeshBatch& mMeshBatch;
	MetadataComponent& mMetadataComponent;
	ColorComponent& mColorComponent;
	SkeletonComponent& mSkeletonComponent;
	nanogui::Matrix4f mModelMatrix;
};
