#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

class Animation {
public:
	struct KeyFrame {
		float time;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
	};
	
	struct BoneAnimation {
		std::string bone_name;
		std::vector<KeyFrame> keyframes;
	};
	
	// Add keyframes for a specific bone
	void add_bone_keyframes(const std::string& bone_name, const std::vector<KeyFrame>& keyframes) {
		m_bone_animations.push_back({bone_name, keyframes});
	}
	
	// Get keyframes for a bone by name
	const std::vector<KeyFrame>* get_bone_keyframes(const std::string& bone_name) const {
		for (const auto& bone_anim : m_bone_animations) {
			if (bone_anim.bone_name == bone_name) {
				return &bone_anim.keyframes;
			}
		}
		return nullptr;  // Bone not found
	}
	
	// Set the total duration of the animation
	void set_duration(float duration) {
		m_duration = duration;
	}
	
	// Get the total duration of the animation
	float get_duration() const {
		return m_duration;
	}
	
private:
	std::vector<BoneAnimation> m_bone_animations;
	float m_duration = 0.0f;  // Duration of the animation
};

