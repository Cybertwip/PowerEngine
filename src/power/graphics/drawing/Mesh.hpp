#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>

class ColorComponent;
class IMeshBatch;
class ShaderManager;

class Mesh : public Drawable {
public:
	Mesh(MeshData& meshData, ShaderWrapper& shader, IMeshBatch& meshBatch, MetadataComponent& metadataComponent, ColorComponent& colorComponent);
	~Mesh() override;
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	ShaderWrapper& get_shader() const { return mShader; }
 
	// Getters for flattened data
	const std::vector<float>& get_flattened_positions() const { return mFlattenedPositions; }
	const std::vector<float>& get_flattened_normals() const { return mFlattenedNormals; }
	const std::vector<float>& get_flattened_tex_coords1() const { return mFlattenedTexCoords1; }
	const std::vector<float>& get_flattened_tex_coords2() const { return mFlattenedTexCoords2; }
	const std::vector<int>& get_flattened_material_ids() const { return mFlattenedMaterialIds; }
	const std::vector<float>& get_flattened_colors() const { return mFlattenedColors; }

	MeshData& get_mesh_data() {
		return mMeshData;
	}
	
	MetadataComponent& get_metadata_component() const {
		return mMetadataComponent;
	}
	
	ColorComponent& get_color_component() const {
		return mColorComponent;
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
	std::vector<int> mFlattenedMaterialIds;
	std::vector<float> mFlattenedColors; // Added to store flattened color data

	IMeshBatch& mMeshBatch;
	MetadataComponent& mMetadataComponent;
	ColorComponent& mColorComponent;
	
	nanogui::Matrix4f mModelMatrix;
};
