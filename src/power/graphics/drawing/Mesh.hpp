#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>

class ColorComponent;
class MeshBatch;
class ShaderManager;

class Mesh : public Drawable {
public:
	class Vertex {
	private:
		static constexpr int MAX_BONE_INFLUENCE = 4;
		
	public:
		Vertex();
		Vertex(const glm::vec3 &pos, const glm::vec2 &tex);
		
		// Bone and weight setters
		void set_bone(int boneId, float weight);
		void set_position(const glm::vec3 &vec);
		void set_normal(const glm::vec3 &vec);
		void set_color(const glm::vec4 &vec);
		void set_texture_coords1(const glm::vec2 &vec);
		void set_texture_coords2(const glm::vec2 &vec);
		
		// New method for setting texture ID
		void set_texture_id(int textureId);
		
		// Accessors
		glm::vec3 get_position() const;
		glm::vec3 get_normal() const;
		glm::vec4 get_color() const;
		glm::vec2 get_tex_coords1() const;
		glm::vec2 get_tex_coords2() const;
		std::array<int, MAX_BONE_INFLUENCE> get_bone_ids() const;
		std::array<float, MAX_BONE_INFLUENCE> get_weights() const;
		int get_texture_id() const;  // New method to get texture ID
		
	private:
		void init_bones();
		
	private:
		glm::vec3 mPosition;
		glm::vec3 mNormal;
		glm::vec4 mColor;
		glm::vec2 mTexCoords1;
		glm::vec2 mTexCoords2;
		std::array<int, Vertex::MAX_BONE_INFLUENCE> mBoneIds;
		std::array<float, Vertex::MAX_BONE_INFLUENCE> mWeights;
		
		// New member to store texture ID
		int mTextureId;
	};


    struct MeshData {
        std::vector<Vertex> mVertices;
        std::vector<unsigned int> mIndices;
        std::vector<std::shared_ptr<MaterialProperties>> mMaterials;
    };

    class MeshShader : public ShaderWrapper {
    public:
        MeshShader(ShaderManager& shader);
    };


public:
	Mesh(std::unique_ptr<MeshData> meshData, ShaderWrapper& shader, MeshBatch& meshBatch, ColorComponent& colorComponent);
	~Mesh() override = default;
    
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

	const MeshData& get_mesh_data() {
		return *mMeshData;
	}
	
	ColorComponent& get_color_component() const {
		return mColorComponent;
	}
	
	const nanogui::Matrix4f& get_model_matrix() {
		return mModelMatrix;
	}
	
	
private:
	std::unique_ptr<MeshData> mMeshData;

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

	MeshBatch& mMeshBatch;
	ColorComponent& mColorComponent;
	
	nanogui::Matrix4f mModelMatrix;
	
	static std::unique_ptr<nanogui::Texture> mDummyTexture;
};
