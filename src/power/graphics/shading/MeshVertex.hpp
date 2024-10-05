#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <glm/glm.hpp>
#include <array>
#include <algorithm> // For std::any_of, std::partial_sort
#include <numeric>   // For std::accumulate
#include <vector>

class MeshVertex {
public:
	MeshVertex();
	MeshVertex(const glm::vec3& pos, const glm::vec2& tex);
	
	MeshVertex(const MeshVertex& other)
	: mPosition(other.mPosition),
	mNormal(other.mNormal),
	mColor(other.mColor),
	mTexCoords1(other.mTexCoords1),
	mTexCoords2(other.mTexCoords2),
	mMaterialId(other.mMaterialId) {
		// No additional logic required since all members are copied
	}
	
	virtual ~MeshVertex() {}
	
	// Setters
	void set_position(const glm::vec3& vec);
	void set_normal(const glm::vec3& vec);
	void set_color(const glm::vec4& vec);
	void set_texture_coords1(const glm::vec2& vec);
	void set_texture_coords2(const glm::vec2& vec);
	void set_material_id(int materialId);
	
	// Accessors
	glm::vec3 get_position() const;
	glm::vec3 get_normal() const;
	glm::vec4 get_color() const;
	glm::vec2 get_tex_coords1() const;
	glm::vec2 get_tex_coords2() const;
	int get_material_id() const;
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		serializer.write_vec3(mPosition);
		serializer.write_vec3(mNormal);
		serializer.write_vec4(mColor);
		serializer.write_vec2(mTexCoords1);
		serializer.write_vec2(mTexCoords2);
		serializer.write_int32(mMaterialId);
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		if (!deserializer.read_vec3(mPosition)) return false;
		if (!deserializer.read_vec3(mNormal)) return false;
		if (!deserializer.read_vec4(mColor)) return false;
		if (!deserializer.read_vec2(mTexCoords1)) return false;
		if (!deserializer.read_vec2(mTexCoords2)) return false;
		if (!deserializer.read_int32(mMaterialId)) return false;
		return true;
	}
	
private:
	glm::vec3 mPosition;
	glm::vec3 mNormal;
	glm::vec4 mColor;
	glm::vec2 mTexCoords1;
	glm::vec2 mTexCoords2;
	int mMaterialId;
};

class SkinnedMeshVertex : public MeshVertex {
public:
	static constexpr int MAX_BONE_INFLUENCE = 4;
	
	SkinnedMeshVertex() {
		mBoneIds.fill(-1);
		mWeights.fill(0.0f);
	}
	
	SkinnedMeshVertex(const MeshVertex& meshVertex) : MeshVertex(meshVertex) {
		mBoneIds.fill(-1);
		mWeights.fill(0.0f);
	}
	
	~SkinnedMeshVertex() override = default;
	
	// Adds a bone weight
	void add_bone_weight(int boneId, float weight) {
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
			if (mBoneIds[i] == boneId) {
				mWeights[i] += weight; // Accumulate weight if bone already exists
				return;
			}
		}
		
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
			if (mBoneIds[i] == -1) {
				mBoneIds[i] = boneId;
				mWeights[i] = weight;
				return;
			}
		}
		
		// If all slots are full, replace the bone with the smallest weight
		int minIndex = std::min_element(mWeights.begin(), mWeights.end()) - mWeights.begin();
		if (weight > mWeights[minIndex]) {
			mBoneIds[minIndex] = boneId;
			mWeights[minIndex] = weight;
		}
	}
	
	// Normalize weights and limit the number of influences
	void normalize_weights(int maxInfluences) {
		// Create a vector of indices
		std::vector<int> indices(MAX_BONE_INFLUENCE);
		std::iota(indices.begin(), indices.end(), 0);
		
		// Sort indices based on weights in descending order
		std::partial_sort(indices.begin(), indices.begin() + maxInfluences, indices.end(),
						  [&](int a, int b) { return mWeights[a] > mWeights[b]; });
		
		// Keep only the top maxInfluences
		std::array<int, MAX_BONE_INFLUENCE> newBoneIds;
		std::array<float, MAX_BONE_INFLUENCE> newWeights;
		newBoneIds.fill(-1);
		newWeights.fill(0.0f);
		
		float totalWeight = 0.0f;
		for (int i = 0; i < maxInfluences; ++i) {
			int idx = indices[i];
			if (mWeights[idx] > 0.0f) {
				newBoneIds[i] = mBoneIds[idx];
				newWeights[i] = mWeights[idx];
				totalWeight += mWeights[idx];
			}
		}
		
		// Normalize weights
		if (totalWeight > 0.0f) {
			for (int i = 0; i < maxInfluences; ++i) {
				newWeights[i] /= totalWeight;
			}
		}
		
		mBoneIds = newBoneIds;
		mWeights = newWeights;
	}
	
	// Method to check if the vertex has no bone influences
	bool has_no_bones() const {
		// Returns true if all weights are zero or bone IDs are -1
		return std::all_of(mWeights.begin(), mWeights.end(), [](float w) { return w <= 0.0f; }) ||
		std::all_of(mBoneIds.begin(), mBoneIds.end(), [](int id) { return id == -1; });
	}
	
	// Accessors
	const std::array<int, MAX_BONE_INFLUENCE>& get_bone_ids() const {
		return mBoneIds;
	}
	
	const std::array<float, MAX_BONE_INFLUENCE>& get_weights() const {
		return mWeights;
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
