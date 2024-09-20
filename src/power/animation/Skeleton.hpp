#pragma once

#include <string>
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

class Skeleton {
public:
	struct Bone {
		std::string name;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		int parent_index;  // Index of the parent bone, -1 if root
	};
	
	Skeleton() = default;
	
	// Add a bone to the skeleton
	void add_bone(const std::string& name, const glm::vec3& translation, const glm::quat& rotation,
				  const glm::vec3& scale, int parent_index) {
		m_bones.push_back({name, translation, rotation, scale, parent_index});
	}
	
	// Get the number of bones
	int num_bones() const {
		return static_cast<int>(m_bones.size());
	}
	
	// Get bone by index
	const Bone& get_bone(int index) const {
		return m_bones[index];
	}
	
private:
	std::vector<Bone> m_bones;
};
