#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>

class ColorComponent;
class ShaderManager;

class SkinnedMesh : public Drawable {
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
		void set_texture_coords1(const glm::vec2 &vec);
		void set_texture_coords2(const glm::vec2 &vec);
		
		// New method for setting texture ID
		void set_texture_id(int textureId);
		
		// Accessors
		glm::vec3 get_position() const;
		glm::vec3 get_normal() const;
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

    class SkinnedMeshShader : public ShaderWrapper {
    public:
        SkinnedMeshShader(ShaderManager& shader);
    };
	
	class MeshBatch {
	private:
		struct VertexIndexer {
			size_t mVertexOffset = 0;
			size_t mIndexOffset = 0;
		};
		
	public:
		MeshBatch(nanogui::RenderPass& renderPass);
		
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
		std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
		
		// Offset tracking
		std::unordered_map<int, std::vector<size_t>> mMeshStartIndices;
		
		std::unordered_map<int, VertexIndexer> mVertexIndexingMap;
		
		nanogui::RenderPass& mRenderPass;
	};


public:
    SkinnedMesh(std::unique_ptr<MeshData> meshData, ShaderWrapper& shader, MeshBatch& meshBatch, ColorComponent& colorComponent);
	~SkinnedMesh() override = default;
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	ShaderWrapper& get_shader() const { return mShader; }
    
	static void init_dummy_texture();

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

	MeshBatch& mMeshBatch;
	ColorComponent& mColorComponent;
	
	nanogui::Matrix4f mModelMatrix;
	
	static std::unique_ptr<nanogui::Texture> mDummyTexture;
};
