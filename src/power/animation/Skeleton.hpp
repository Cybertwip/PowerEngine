#pragma once

#include "IBone.hpp"
#include "ISkeleton.hpp"

#include "filesystem/CompressedSerialization.hpp"

#include <components/TransformComponent.hpp>


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

class Skeleton : public ISkeleton {
public:
	class Bone : public IBone {
	public:
		std::string name;
		Bone* parent;
		int index;  // -1 if root
		int parent_index;  // -1 if root
		glm::mat4 offset;
		TransformComponent transform;
		glm::mat4 global;
		std::vector<int> children;
		
		Bone() : parent(nullptr), index(-1), parent_index(-1) {
			
		}

		Bone(const std::string& name, Bone* parent, int index, int parent_index, const glm::mat4& offset, const glm::mat4& bindpose)
		: name(name), parent(parent), index(index), parent_index(parent_index), offset(offset), transform(bindpose), global(1.0f) {}
		
		~Bone() = default;
		
		// Serialize method for Bone
		void serialize(CompressedSerialization::Serializer& serializer) const {
			serializer.write_string(name);
			serializer.write_int32(index);
			serializer.write_int32(parent_index);
			serializer.write_mat4(offset);
			serializer.write_mat4(transform.get_matrix());
			serializer.write_mat4(transform.get_matrix());
			serializer.write_mat4(global);

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
			
			glm::mat4 bindpose;
			if (!deserializer.read_mat4(bindpose)) return false;
			
			glm::mat4 dummy; // @TODO remove
			if (!deserializer.read_mat4(dummy)) return false;
			
			transform = TransformComponent(bindpose);
			
			// @TODO remove
			if (!deserializer.read_mat4(global)) return false;

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
		
		IBone* get_parent() override {
			return parent;
		}
		
		std::string get_name() override {
			return name;
		}
		
		int get_parent_index() override {
			return parent_index;
		}
		
		virtual const std::vector<int> get_child_indices() const override {
			return children;
		}

		void set_translation(const glm::vec3& translation) override {
			transform.set_translation(translation);
		}
		
		void set_rotation(const glm::quat& rotation) override {
			transform.set_rotation(rotation);
		}
		
		void set_scale(const glm::vec3& scale) override {
			transform.set_scale(scale);
		}
		
		glm::vec3 get_translation() const override {
			return transform.get_translation();
		}
		
		glm::vec3 get_scale() const override {
			return transform.get_scale();
		}
		
		glm::quat get_rotation() const override {
			return transform.get_rotation();
		}
		
		glm::mat4 get_transform_matrix() const override {
			return transform.get_matrix();
		}

		void remove_child_index(int child_index){
			children.erase(std::remove(children.begin(), children.end(), child_index), children.end());
		}
	};
	
	Skeleton() = default;
	~Skeleton() = default;
	
	void add_bone(const std::string& name, const glm::mat4& offset, const glm::mat4& bindpose, int parent_index) {
		int new_bone_index = static_cast<int>(m_bones.size());
		
		if (parent_index == -1) {
			m_bones.push_back(std::make_unique<Bone>(name, nullptr, new_bone_index, parent_index, offset, bindpose));
		} else {
			m_bones.emplace_back(std::make_unique<Bone>(name, m_bones[parent_index].get(), new_bone_index, parent_index, offset, bindpose));
		}
		
		if (parent_index != -1 && parent_index != new_bone_index) { // root can't have itself as child
			assert(parent_index >= 0 && parent_index < new_bone_index);
			m_bones[parent_index]->children.push_back(new_bone_index);
		}
	}
	
	// Get the number of bones
	int num_bones() const override {
		return static_cast<int>(m_bones.size());
	}
		
	// Get mutable bone by index
	IBone& get_bone(int index) override {
		assert(index >= 0 && index < num_bones());
		return *m_bones[index];
	}
	
	// Get mutable bone by index
	int get_bone_index(IBone& bone) override {
		// Loop through the m_bones vector to find the bone's index
		for (int i = 0; i < m_bones.size(); ++i) {
			if (m_bones[i] != nullptr && m_bones[i].get() == &bone) {  // Assuming IBone has an == operator
				return i;
			}
		}
		
		// If bone is not found, return an error code or handle appropriately
		return -1; // Indicating bone not found
	}

	void trim_bone(IBone& bone) override {
		// Use remove-erase idiom with remove_if to remove the unique_ptr pointing to &bone
		m_bones.erase(
					  std::remove_if(m_bones.begin(), m_bones.end(), [&](const std::unique_ptr<IBone>& ptr) {
						  return ptr.get() == &bone;
					  }),
					  m_bones.end()
					  );
	}

	// Find bone by name
	IBone* find_bone(const std::string& name) {
		for (auto& bone : m_bones) {
			if (bone->name == name) {
				return bone.get();
			}
		}
		return nullptr;
	}
	
	glm::mat4 get_bone_bindpose(int index) {
		assert(index >= 0 && index < num_bones());
		return m_bones[index]->get_transform_matrix();
	}
	
	void compute_offsets(const std::vector<glm::mat4>& withAnimation) {
		if (m_bones.empty()) return;
		
		for (auto& bone : m_bones) {
			if (bone->parent_index == -1) {
				glm::mat4 identity = glm::mat4(1.0f);
				compute_global_and_transform(*bone, identity, withAnimation);
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
			bone->serialize(serializer);
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
		std::vector<std::unique_ptr<Bone>> tempBones;
		
		// Deserialize each bone
		for (uint32_t i = 0; i < numBones; ++i) {
			auto& bone = tempBones.emplace_back(std::make_unique<Bone>());
			if (!bone->deserialize(deserializer)) return false;
		}
		
		// Assign deserialized bones to m_bones
		m_bones = std::move(tempBones);
		
		// Optionally, verify the integrity of the skeleton (e.g., no cycles, valid parent indices)
		// This step is skipped for brevity but recommended in a production environment.
		
		return true;
	}

	const std::vector<std::unique_ptr<Bone>>& get_bones() {
		return m_bones;
	}
	
private:
	std::vector<std::unique_ptr<Bone>> m_bones;
	
	void compute_global_and_transform(Bone& bone, const glm::mat4& parentGlobal, const std::vector<glm::mat4>& withAnimation) {
		
		// Start with the parent's global transformation
		glm::mat4 global = parentGlobal;
		
		// Apply the bone's bind pose first
		global *= bone.get_transform_matrix();
		
		glm::mat4 transformation = glm::mat4(1.0f);
		
		if (!withAnimation.empty()) {
			
			if (bone.index < withAnimation.size() && bone.index >= 0) {
				transformation = withAnimation[bone.index];
			}
		}
		
		global *= transformation;

		bone.global = global * bone.offset;
		
		for (int childIndex : bone.children) {
			compute_global_and_transform(*m_bones[childIndex], global, withAnimation);
		}
	}
};
