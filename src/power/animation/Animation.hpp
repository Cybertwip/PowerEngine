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
	void set_duration(int duration) {
		m_duration = duration;
	}
	
	// Get the total duration of the animation
	int get_duration() const {
		return m_duration;
	}
	
	// Evaluate the animation for a specific time, returning the transform for each bone
	std::vector<glm::mat4> evaluate(float time) const {
		std::vector<glm::mat4> bone_transforms;
		
		// For each bone animation
		for (const auto& bone_anim : m_bone_animations) {
			const auto& keyframes = bone_anim.keyframes;
			
			// If there are no keyframes, continue to the next bone
			if (keyframes.empty()) {
				continue;
			}
			
			// If the time is before the first keyframe, return the first keyframe
			if (time <= keyframes.front().time) {
				const KeyFrame& first_keyframe = keyframes.front();
				
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), first_keyframe.translation);
				glm::mat4 rotation_matrix = glm::mat4_cast(first_keyframe.rotation);
				glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), first_keyframe.scale);
				
				glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
				bone_transforms.push_back(bone_transform);
				continue;
			}
			
			// If the time is after the last keyframe, return the last keyframe
			if (time >= keyframes.back().time) {
				const KeyFrame& last_keyframe = keyframes.back();
				
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), last_keyframe.translation);
				glm::mat4 rotation_matrix = glm::mat4_cast(last_keyframe.rotation);
				glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), last_keyframe.scale);
				
				glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
				bone_transforms.push_back(bone_transform);
				continue;
			}
			
			// Find the two keyframes surrounding the given time
			auto it1 = std::find_if(keyframes.begin(), keyframes.end(), [&](const KeyFrame& kf) {
				return kf.time >= time;
			});
			
			auto it0 = (it1 != keyframes.begin()) ? std::prev(it1) : it1;
			
			// Interpolate between keyframes
			float t = (time - it0->time) / (it1->time - it0->time);
			
			glm::vec3 interpolated_translation = glm::mix(it0->translation, it1->translation, t);
			glm::quat interpolated_rotation = glm::slerp(it0->rotation, it1->rotation, t);
			glm::vec3 interpolated_scale = glm::mix(it0->scale, it1->scale, t);
			
			// Construct the transformation matrix
			glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), interpolated_translation);
			glm::mat4 rotation_matrix = glm::mat4_cast(interpolated_rotation);
			glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), interpolated_scale);
			
			glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
			bone_transforms.push_back(bone_transform);
		}
		
		return bone_transforms;
	}
private:
	std::vector<BoneAnimation> m_bone_animations;
	int m_duration = 0;  // Duration of the animation
};
