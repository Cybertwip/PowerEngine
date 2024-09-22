#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

class Skeleton {
public:
	struct Bone {
		std::string name;
		int index;  // -1 if root
		int parent_index;  // -1 if root
		glm::mat4 offset;
		glm::mat4 bindpose;
		glm::mat4 local;
		glm::mat4 global;
		glm::mat4 transform;
		std::vector<int> children;
		
		Bone(const std::string& name, int index, int parent_index, const glm::mat4& offset, const glm::mat4& bindpose, const glm::mat4& local)
		: name(name), index(index), parent_index(parent_index), offset(offset), bindpose(bindpose), local(local), global(1.0f), transform(1.0f) {}
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
	
	void compute_offsets(const std::vector<glm::mat4>& withAnimation = {}) {
		if (m_bones.empty()) return;
		
		if (!withAnimation.empty()) {
			assert(withAnimation.size() == m_bones.size() && "Unmatched animations and bones size");
		}
		
		for (Bone& bone : m_bones) {
			if (bone.parent_index == -1) {
				glm::mat4 identity = glm::mat4(1.0f);
				compute_global_and_transform(bone, identity, withAnimation);
			}
		}
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
			transformation = withAnimation[bone.index];
		}
				
		global *= bone.local * transformation;
		
		bone.transform = global * bone.offset;
		
		for (int childIndex : bone.children) {
			compute_global_and_transform(m_bones[childIndex], global, withAnimation);
		}
	}
};
