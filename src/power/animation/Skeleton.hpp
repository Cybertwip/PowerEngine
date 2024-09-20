#pragma once

class Skeleton {
public:
	struct Bone {
		std::string name;
		Transform local_transform;    // Local transformation relative to parent
		int parent_index;            // Index of the parent bone, -1 if root
		glm::mat4 global_transform;  // Precomputed global transformation
		glm::mat4 inverse_bind_pose; // Inverse bind pose matrix
	};
	
	Skeleton() = default;
	
	// Add a bone to the skeleton
	void add_bone(const std::string& name, const glm::vec3& translation, const glm::quat& rotation,
				  const glm::vec3& scale, int parent_index) {
		m_bones.emplace_back(Bone{
			name,
			Transform{translation, rotation, scale},
			parent_index,
			glm::mat4(1.0f),
			glm::mat4(1.0f) // Initialize inverse_bind_pose to identity
		});
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
	
	// Precompute global transformations for all bones
	void compute_global_transforms() {
		for (size_t i = 0; i < m_bones.size(); ++i) {
			compute_bone_global_transform(static_cast<int>(i));
		}
	}
	
	// Compute inverse bind poses after global transforms are computed
	void compute_inverse_bind_poses() {
		for (auto& bone : m_bones) {
			bone.inverse_bind_pose = glm::inverse(bone.global_transform);
		}
	}
	
private:
	std::vector<Bone> m_bones;
	
	// Helper function to compute global transform of a single bone
	void compute_bone_global_transform(int bone_index) {
		Bone& bone = m_bones[bone_index];
		glm::mat4 local_matrix = bone.local_transform.to_matrix();
		
		if (bone.parent_index == -1) {
			// Root bone: global transform is the local transform
			bone.global_transform = local_matrix;
		} else {
			// Child bone: global transform is parent's global transform * local transform
			glm::mat4 parent_global = m_bones[bone.parent_index].global_transform;
			bone.global_transform = parent_global * local_matrix;
		}
	}
};
