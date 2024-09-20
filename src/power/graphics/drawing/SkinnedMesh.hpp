#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>

class ColorComponent;
class SkinnedMeshBatch;
class ShaderManager;

class SkinnedMesh : public Drawable {
public:
	SkinnedMesh(std::unique_ptr<SkinnedMeshData> skinnedMeshData, ShaderWrapper& shader, SkinnedMeshBatch& meshBatch, ColorComponent& colorComponent);
	~SkinnedMesh() override = default;
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	ShaderWrapper& get_shader() const { return mShader; }
	
	// Getters for flattened data
	const std::vector<float>& get_flattened_positions() const { return mFlattenedPositions; }
	const std::vector<float>& get_flattened_normals() const { return mFlattenedNormals; }
	const std::vector<float>& get_flattened_tex_coords1() const { return mFlattenedTexCoords1; }
	const std::vector<float>& get_flattened_tex_coords2() const { return mFlattenedTexCoords2; }
	const std::vector<int>& get_flattened_bone_ids() const { return mFlattenedBoneIds; }
	const std::vector<float>& get_flattened_weights() const { return mFlattenedWeights; }
	const std::vector<int>& get_flattened_texture_ids() const { return mFlattenedTextureIds; }
	const std::vector<float>& get_flattened_colors() const { return mFlattenedColors; }
	
	SkinnedMeshData& get_mesh_data() {
		return *mSkinnedMeshData;
	}
	
	ColorComponent& get_color_component() const {
		return mColorComponent;
	}
	
	const nanogui::Matrix4f& get_model_matrix() {
		return mModelMatrix;
	}
	
	
private:
	std::unique_ptr<SkinnedMeshData> mSkinnedMeshData;
	
	ShaderWrapper& mShader;
	
	// Flattened data
	std::vector<float> mFlattenedPositions;
	std::vector<float> mFlattenedNormals;
	std::vector<float> mFlattenedTexCoords1;
	std::vector<float> mFlattenedTexCoords2;
	std::vector<int> mFlattenedBoneIds;
	std::vector<float> mFlattenedWeights;
	std::vector<int> mFlattenedTextureIds;
	std::vector<float> mFlattenedColors; // Added to store flattened color data
	
	SkinnedMeshBatch& mMeshBatch;
	ColorComponent& mColorComponent;
	
	nanogui::Matrix4f mModelMatrix;
	
	static std::unique_ptr<nanogui::Texture> mDummyTexture;
};
