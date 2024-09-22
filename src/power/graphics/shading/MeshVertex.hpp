#pragma once

#include <glm/glm.hpp>
#include <array>
#include <algorithm> // For std::any_of


class MeshVertex {
public:
	MeshVertex();
	MeshVertex(const glm::vec3 &pos, const glm::vec2 &tex);
	
	// Bone and weight setters
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
	int get_texture_id() const;  // New method to get texture ID
	
private:
	glm::vec3 mPosition;
	glm::vec3 mNormal;
	glm::vec4 mColor;
	glm::vec2 mTexCoords1;
	glm::vec2 mTexCoords2;
	// New member to store texture ID
	int mTextureId;
};

class SkinnedMeshVertex {
public:
	static constexpr int MAX_BONE_INFLUENCE = 4;
	
	explicit SkinnedMeshVertex(MeshVertex& meshVertex) : mMeshVertex(meshVertex) {
		mBoneIds.fill(-1);
		mWeights.fill(0.0f);
	}
	
	// Assigns a bone and its weight to the first available slot
	void set_bone(int boneId, float weight) {
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			if (mBoneIds[i] < 0)
			{
				mBoneIds[i] = boneId;
				mWeights[i] = weight;
				return;
			}
		}
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			if (mWeights[i] < weight)
			{
				mBoneIds[i] = boneId;
				mWeights[i] = weight;
				return;
			}
		}

	}
	
	// Setters forwarding to MeshVertex
	void set_position(const glm::vec3 &vec) {
		mMeshVertex.set_position(vec);
	}
	
	void set_normal(const glm::vec3 &vec) {
		mMeshVertex.set_normal(vec);
	}
	
	void set_color(const glm::vec4 &vec) {
		mMeshVertex.set_color(vec);
	}
	
	void set_texture_coords1(const glm::vec2 &vec) {
		mMeshVertex.set_texture_coords1(vec);
	}
	
	void set_texture_coords2(const glm::vec2 &vec) {
		mMeshVertex.set_texture_coords2(vec);
	}
	
	// Forward texture ID setter
	void set_texture_id(int textureId) {
		mMeshVertex.set_texture_id(textureId);
	}
	
	// Accessors forwarding
	glm::vec3 get_position() const {
		return mMeshVertex.get_position();
	}
	
	glm::vec3 get_normal() const {
		return mMeshVertex.get_normal();
	}
	
	glm::vec4 get_color() const {
		return mMeshVertex.get_color();
	}
	
	glm::vec2 get_tex_coords1() const {
		return mMeshVertex.get_tex_coords1();
	}
	
	glm::vec2 get_tex_coords2() const {
		return mMeshVertex.get_tex_coords2();
	}
	
	int get_texture_id() const {
		return mMeshVertex.get_texture_id();
	}
	
	// Returns a const reference to the bone IDs
	const std::array<int, MAX_BONE_INFLUENCE>& get_bone_ids() const {
		return mBoneIds;
	}
	
	// Returns a const reference to the weights
	const std::array<float, MAX_BONE_INFLUENCE>& get_weights() const {
		return mWeights;
	}
	
	// Method to check if the vertex has no bone influences
	bool has_no_bones() const {
		// Returns true if all weights are zero
		return !std::any_of(mWeights.begin(), mWeights.end(), [](float w) { return w > 0.0f; });
	}
	
private:
	MeshVertex mMeshVertex;
	
	std::array<int, MAX_BONE_INFLUENCE> mBoneIds;
	std::array<float, MAX_BONE_INFLUENCE> mWeights;
};
