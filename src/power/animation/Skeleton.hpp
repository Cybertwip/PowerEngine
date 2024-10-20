#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

#include <fstream>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream> // For error messages

class Skeleton {
public:
	struct Bone {
		std::string name;
		int index;  // -1 if root
		int parent_index;  // -1 if root
		glm::mat4 offset;
		glm::mat4 bindpose;
		glm::mat4 local;
		glm::mat4 transform;
		std::vector<int> children;
		
		Bone(){
			
		}

		Bone(const std::string& name, int index, int parent_index, const glm::mat4& offset, const glm::mat4& bindpose, const glm::mat4& local)
		: name(name), index(index), parent_index(parent_index), offset(offset), bindpose(bindpose), local(local), transform(1.0f) {}
		
		// Serialize method for Bone
		void serialize(CompressedSerialization::Serializer& serializer) const {
			serializer.write_string(name);
			serializer.write_int32(index);
			serializer.write_int32(parent_index);
			serializer.write_mat4(offset);
			serializer.write_mat4(bindpose);
			serializer.write_mat4(local);
			serializer.write_mat4(transform);
			
			// Serialize number of children
			uint32_t numChildren = static_cast<uint32_t>(children.size());
			serializer.write_uint32(numChildren);
			
			// Serialize each child index
			for (const auto& childIndex : children) {
				serializer.write_int32(childIndex);
			}
		}
		
		// Deserialize method for Bone
		bool deserialize(CompressedSerialization::Deserializer& deserializer) {
			if (!deserializer.read_string(name)) return false;
			if (!deserializer.read_int32(index)) return false;
			if (!deserializer.read_int32(parent_index)) return false;
			if (!deserializer.read_mat4(offset)) return false;
			if (!deserializer.read_mat4(bindpose)) return false;
			if (!deserializer.read_mat4(local)) return false;
			if (!deserializer.read_mat4(transform)) return false;
			
			// Deserialize number of children
			uint32_t numChildren = 0;
			if (!deserializer.read_uint32(numChildren)) return false;
			
			// Deserialize each child index
			children.resize(numChildren);
			for (uint32_t i = 0; i < numChildren; ++i) {
				if (!deserializer.read_int32(children[i])) return false;
			}
			
			return true;
		}

	};
	
	
	Skeleton() = default;
	
	void add_bone(const std::string& name, const glm::mat4& offset, const glm::mat4& bindpose, int parent_index) {
		int new_bone_index = static_cast<int>(m_bones.size());
		m_bones.emplace_back(name, new_bone_index, parent_index, offset, bindpose, glm::identity<glm::mat4>());
		
		if (parent_index != -1) {
			assert(parent_index >= 0 && parent_index < new_bone_index);
			m_bones[parent_index].children.push_back(new_bone_index);
		}
	}
	
	// Get the number of bones
	int num_bones() const {
		return static_cast<int>(m_bones.size());
	}
	
	// Get bone by index
	const Bone& get_bone(int index) const {
		assert(index >= 0 && index < num_bones());
		return m_bones[index];
	}
	
	
	
	// Get mutable bone by index
	Bone& get_bone(int index) {
		assert(index >= 0 && index < num_bones());
		return m_bones[index];
	}
	
	// Find bone by name
	Bone* find_bone(const std::string& name) {
		for (auto& bone : m_bones) {
			if (bone.name == name) {
				return &bone;
			}
		}
		return nullptr;
	}
	
	void compute_offsets(const std::vector<glm::mat4>& withAnimation) {
		if (m_bones.empty()) return;
		
		for (Bone& bone : m_bones) {
			if (bone.parent_index == -1) {
				glm::mat4 identity = glm::mat4(1.0f);
				compute_global_and_transform(bone, identity, withAnimation);
			}
		}
	}
	
	/* @brief Serializes the skeleton data using CompressedSerialization::Serializer.
	*
	* @param serializer The serializer instance to write data into.
	*/
	void serialize(CompressedSerialization::Serializer& serializer) const {
		// Serialize number of bones
		uint32_t numBones = static_cast<uint32_t>(m_bones.size());
		serializer.write_uint32(numBones);
		
		// Serialize each bone
		for (const auto& bone : m_bones) {
			bone.serialize(serializer);
		}
	}
	
	/**
	 * @brief Deserializes the skeleton data using CompressedSerialization::Deserializer.
	 *
	 * @param deserializer The deserializer instance to read data from.
	 * @return true If deserialization was successful.
	 * @return false If an error occurred during deserialization.
	 */
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		// Deserialize number of bones
		uint32_t numBones = 0;
		if (!deserializer.read_uint32(numBones)) return false;
		
		// Clear existing bones
		m_bones.clear();
		m_bones.reserve(numBones);
		
		// Temporary storage for bones to allow setting up children after all bones are read
		std::vector<Bone> tempBones(numBones);
		
		// Deserialize each bone
		for (uint32_t i = 0; i < numBones; ++i) {
			Bone bone;
			if (!bone.deserialize(deserializer)) return false;
			tempBones[i] = bone;
		}
		
		// Assign deserialized bones to m_bones
		m_bones = std::move(tempBones);
		
		// Optionally, verify the integrity of the skeleton (e.g., no cycles, valid parent indices)
		// This step is skipped for brevity but recommended in a production environment.
		
		return true;
	}

	const std::vector<Bone>& get_bones() {
		return m_bones;
	}
	
private:
	std::vector<Bone> m_bones;
	
	void compute_global_and_transform(Bone& bone, const glm::mat4& parentGlobal, const std::vector<glm::mat4>& withAnimation) {
		
		// Start with the parent's global transformation
		glm::mat4 global = parentGlobal;
		
		// Apply the bone's bind pose first
		global *= bone.bindpose;
		
		// Only apply the special rotation to bone index 3
		glm::mat4 transformation = glm::mat4(1.0f);

		if (!withAnimation.empty()) {
			
			if (bone.index < withAnimation.size()) {
				transformation = withAnimation[bone.index];
			}
		}
				
		global *= bone.local * transformation;
		
		bone.transform = global * bone.offset;
		
		for (int childIndex : bone.children) {
			compute_global_and_transform(m_bones[childIndex], global, withAnimation);
		}
	}
	
};
