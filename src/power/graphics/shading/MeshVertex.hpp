#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <glm/glm.hpp>
#include <array>
#include <algorithm> // For std::any_of

class MeshVertex {
public:
	MeshVertex();
	MeshVertex(const glm::vec3 &pos, const glm::vec2 &tex);
	
	MeshVertex(const MeshVertex& other)
	: mPosition(other.mPosition),   // Copy position
	mNormal(other.mNormal),       // Copy normal
	mColor(other.mColor),         // Copy color
	mTexCoords1(other.mTexCoords1), // Copy texture coordinates 1
	mTexCoords2(other.mTexCoords2), // Copy texture coordinates 2
	mTextureId(other.mTextureId)   // Copy texture ID
	{
		// No additional logic required since all members are copied
	}

	
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
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		serializer.write_vec3(mPosition);
		serializer.write_vec3(mNormal);
		serializer.write_vec4(mColor);
		serializer.write_vec2(mTexCoords1);
		serializer.write_vec2(mTexCoords2);
		serializer.write_int32(mTextureId);
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		if (!deserializer.read_vec3(mPosition)) return false;
		if (!deserializer.read_vec3(mNormal)) return false;
		if (!deserializer.read_vec4(mColor)) return false;
		if (!deserializer.read_vec2(mTexCoords1)) return false;
		if (!deserializer.read_vec2(mTexCoords2)) return false;
		if (!deserializer.read_int32(mTextureId)) return false;
		return true;
	}

	
private:
	glm::vec3 mPosition;
	glm::vec3 mNormal;
	glm::vec4 mColor;
	glm::vec2 mTexCoords1;
	glm::vec2 mTexCoords2;
	// New member to store texture ID
	int mTextureId;
};

class SkinnedMeshVertex : public MeshVertex {
public:
	static constexpr int MAX_BONE_INFLUENCE = 4;
	
	SkinnedMeshVertex() {
		mBoneIds.fill(-1);
		mWeights.fill(0.0f);
	}
	
	 SkinnedMeshVertex(MeshVertex& meshVertex) : MeshVertex(meshVertex) {
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
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		MeshVertex::serialize(serializer);
		for (const int& boneId : mBoneIds) {
			serializer.write_int32(boneId);
		}
		for (const float& weight : mWeights) {
			serializer.write_float(weight);
		}
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		if (!MeshVertex::deserialize(deserializer)) return false;
		for (int& boneId : mBoneIds) {
			if (!deserializer.read_int32(boneId)) return false;
		}
		for (float& weight : mWeights) {
			if (!deserializer.read_float(weight)) return false;
		}
		return true;
	}

	
private:
	std::array<int, MAX_BONE_INFLUENCE> mBoneIds;
	std::array<float, MAX_BONE_INFLUENCE> mWeights;
};
