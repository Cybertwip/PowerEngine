#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <algorithm>
#include <string>
#include <fstream>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream> // For error messages

class Animation {
public:
	struct KeyFrame {
		float time;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		
		// Serialize method for KeyFrame
		void serialize(CompressedSerialization::Serializer& serializer) const {
			serializer.write_float(time);
			serializer.write_vec3(translation);
			serializer.write_quat(rotation);
			serializer.write_vec3(scale);
		}
		
		// Deserialize method for KeyFrame
		bool deserialize(CompressedSerialization::Deserializer& deserializer) {
			if (!deserializer.read_float(time)) return false;
			if (!deserializer.read_vec3(translation)) return false;
			if (!deserializer.read_quat(rotation)) return false;
			if (!deserializer.read_vec3(scale)) return false;
			return true;
		}

	};
	
	struct BoneAnimation {
		int bone_index;
		std::vector<KeyFrame> keyframes;
		
		// Serialize method for BoneAnimation
		void serialize(CompressedSerialization::Serializer& serializer) const {
			serializer.write_int32(bone_index);
			uint32_t keyframeCount = static_cast<uint32_t>(keyframes.size());
			serializer.write_uint32(keyframeCount);
			for (const auto& keyframe : keyframes) {
				keyframe.serialize(serializer);
			}
		}
		
		// Deserialize method for BoneAnimation
		bool deserialize(CompressedSerialization::Deserializer& deserializer) {
			if (!deserializer.read_int32(bone_index)) return false;
			uint32_t keyframeCount = 0;
			if (!deserializer.read_uint32(keyframeCount)) return false;
			keyframes.resize(keyframeCount);
			for (auto& keyframe : keyframes) {
				if (!keyframe.deserialize(deserializer)) return false;
			}
			return true;
		}

	};
	
	
	/**
	 * Blends two KeyFrames based on the provided blend factor alpha.
	 *
	 * @param kf1   The first KeyFrame.
	 * @param kf2   The second KeyFrame.
	 * @param alpha The blend factor (0.0 = kf1, 1.0 = kf2).
	 * @return      A new KeyFrame that is the blend of kf1 and kf2.
	 */
	static KeyFrame blendKeyframes(const KeyFrame& kf1, const KeyFrame& kf2, float alpha) {
		// Clamp alpha to the range [0, 1] to avoid extrapolation
		alpha = std::clamp(alpha, 0.0f, 1.0f);
		
		KeyFrame blended;
		
		// Linearly interpolate the time
		blended.time = glm::mix(kf1.time, kf2.time, alpha);
		
		// Linearly interpolate the translation and scale vectors
		blended.translation = glm::mix(kf1.translation, kf2.translation, alpha);
		blended.scale = glm::mix(kf1.scale, kf2.scale, alpha);
		
		// Spherically interpolate the rotation quaternions
		blended.rotation = glm::slerp(kf1.rotation, kf2.rotation, alpha);
		
		// Ensure the quaternion is normalized after slerp
		blended.rotation = glm::normalize(blended.rotation);
		
		return blended;
	}

	
	Animation() : m_duration(0) {
		
	}
	
	// Add keyframes for a specific bone
	void add_bone_keyframes(int boneIndex, const std::vector<KeyFrame>& keyframes) {
		m_bone_animations.push_back({boneIndex, keyframes});
	}
	
	bool empty() const {
		return m_bone_animations.empty();
	}
	
	void sort() {
		std::sort(m_bone_animations.begin(), m_bone_animations.end(), [](const BoneAnimation& a, const BoneAnimation& b) {
			return a.bone_index < b.bone_index;
		});
	}

	// Evaluate the animation for a specific time, returning the KeyFrame for each bone
	std::vector<KeyFrame> evaluate_keyframes(float time) const {
		std::vector<KeyFrame> bone_keyframes;
		
		// For each bone animation
		for (const auto& bone_anim : m_bone_animations) {
			const auto& keyframes = bone_anim.keyframes;
			
			// If there are no keyframes, continue to the next bone
			if (keyframes.empty()) {
				continue;
			}
			
			KeyFrame current_keyframe;
			
			// If the time is before the first keyframe, use the first keyframe
			if (time <= keyframes.front().time) {
				current_keyframe = keyframes.front();
			}
			// If the time is after the last keyframe, use the last keyframe
			else if (time >= keyframes.back().time) {
				current_keyframe = keyframes.back();
			}
			else {
				// Find the two keyframes surrounding the given time
				auto it1 = std::find_if(keyframes.begin(), keyframes.end(), [&](const KeyFrame& kf) {
					return kf.time >= time;
				});
				
				auto it0 = (it1 != keyframes.begin()) ? std::prev(it1) : it1;
				
				// Calculate the interpolation factor
				float t = (time - it0->time) / (it1->time - it0->time);
				
				// Interpolate translation, rotation, and scale
				glm::vec3 interpolated_translation = glm::mix(it0->translation, it1->translation, t);
				glm::quat interpolated_rotation = glm::slerp(it0->rotation, it1->rotation, t);
				glm::vec3 interpolated_scale = glm::mix(it0->scale, it1->scale, t);
				
				// Assign interpolated values to the current keyframe
				current_keyframe.time = time;
				current_keyframe.translation = interpolated_translation;
				current_keyframe.rotation = interpolated_rotation;
				current_keyframe.scale = interpolated_scale;
			}
			
			// Add the current keyframe to the list
			bone_keyframes.push_back(current_keyframe);
		}
		
		return bone_keyframes;
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
	
	/**
	 * @brief Serializes the animation data using CompressedSerialization::Serializer.
	 *
	 * @param serializer The serializer instance to write data into.
	 */
	void serialize(CompressedSerialization::Serializer& serializer) const {
		// Serialize duration
		serializer.write_int32(static_cast<int32_t>(m_duration));
		
		// Serialize number of BoneAnimations
		uint32_t boneAnimCount = static_cast<uint32_t>(m_bone_animations.size());
		serializer.write_uint32(boneAnimCount);
		
		// Serialize each BoneAnimation
		for (const auto& bone_anim : m_bone_animations) {
			bone_anim.serialize(serializer);
		}
	}
	
	/**
	 * @brief Deserializes the animation data using CompressedSerialization::Deserializer.
	 *
	 * @param deserializer The deserializer instance to read data from.
	 * @return true If deserialization was successful.
	 * @return false If an error occurred during deserialization.
	 */
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		// Deserialize duration
		if (!deserializer.read_int32(m_duration)) return false;
		
		// Deserialize number of BoneAnimations
		uint32_t boneAnimCount = 0;
		if (!deserializer.read_uint32(boneAnimCount)) return false;
		
		// Deserialize each BoneAnimation
		m_bone_animations.resize(boneAnimCount);
		for (auto& bone_anim : m_bone_animations) {
			if (!bone_anim.deserialize(deserializer)) return false;
		}
		
		return true;
	}

private:
	std::vector<BoneAnimation> m_bone_animations;
	int m_duration = 0;  // Duration of the animation
};
