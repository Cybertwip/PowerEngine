#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <algorithm>

class Animation {
public:
	struct KeyFrame {
		float time;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
	};
	
	struct BoneAnimation {
		int bone_index;
		std::vector<KeyFrame> keyframes;
	};
	
	// Add keyframes for a specific bone
	void add_bone_keyframes(int boneIndex, const std::vector<KeyFrame>& keyframes) {
		m_bone_animations.push_back({boneIndex, keyframes});
	}
	
	void sort() {
		std::sort(m_bone_animations.begin(), m_bone_animations.end(), [](const BoneAnimation& a, const BoneAnimation& b) {
			return a.bone_index < b.bone_index;
		});
	}
	
	// Get keyframes for a bone by name
	const std::vector<KeyFrame>* get_bone_keyframes(int boneIndex) const {
		for (const auto& bone_anim : m_bone_animations) {
			if (bone_anim.bone_index == boneIndex) {
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
	
	// Evaluate the animation for a specific time, returning the transform for each bone
	std::vector<glm::mat4> evaluate(float time) const {
		std::vector<glm::mat4> bone_transforms;
		
		// Clamp time to animation duration
		if (time > m_duration) {
			time = std::fmod(time, m_duration);
		}
		
		// For each bone animation
		for (const auto& bone_anim : m_bone_animations) {
			const auto& keyframes = bone_anim.keyframes;
			
			// Find the first keyframe after or at the given time
			auto it1 = std::find_if(keyframes.begin(), keyframes.end(), [&](const KeyFrame& kf) {
				return kf.time >= time;
			});
			
			// If the first keyframe is beyond the current time, use the previous one
			if (it1 != keyframes.begin() && it1 != keyframes.end()) {
				auto it0 = std::prev(it1);
				
				// Decide whether to use the previous, current, or next keyframe
				const KeyFrame* selected_keyframe = nullptr;
				if (time == it0->time) {
					selected_keyframe = &(*it0); // Exact match with the previous keyframe
				} else if (time == it1->time) {
					selected_keyframe = &(*it1); // Exact match with the current keyframe
				} else if (time < it0->time) {
					selected_keyframe = &(*it0); // Use the previous keyframe if earlier than both
				} else {
					selected_keyframe = &(*it1); // Use the next keyframe
				}
				
				// Construct the transformation matrix without interpolation
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), selected_keyframe->translation);
				glm::mat4 rotation_matrix = glm::mat4_cast(selected_keyframe->rotation);
				glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), selected_keyframe->scale);
				
				// Combine into final bone transform
				glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
				bone_transforms.push_back(bone_transform);
			}
		}
		return bone_transforms;
	}
	
private:
	std::vector<BoneAnimation> m_bone_animations;
	float m_duration = 0.0f;  // Duration of the animation
};
