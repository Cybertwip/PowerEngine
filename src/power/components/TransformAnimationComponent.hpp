#pragma once

#include "AnimationComponent.hpp"
#include "TransformComponent.hpp"

#include "animation/AnimationTimeProvider.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>
#include <algorithm>

class TransformAnimationComponent : public AnimationComponent {
public:
	TransformAnimationComponent(TransformComponent& provider, AnimationTimeProvider& animationTimeProvider) :
	mProvider(provider), mAnimationTimeProvider(animationTimeProvider), mRegistrationId(-1), mFrozen(false) {
		
	}
	
	void TriggerRegistration() override {
		if(mRegistrationId != -1) {
			mProvider
				.unregister_on_transform_changed_callback(mRegistrationId);
			
			mRegistrationId = -1;
		}
		
		mRegistrationId = mProvider.register_on_transform_changed_callback([this](const TransformComponent& transform) {
			if (mFrozen) {
				addKeyframe(mAnimationTimeProvider.GetTime(), transform.get_translation(), transform.get_rotation(), transform.get_scale());
			}
		});
	}
	
	void AddKeyframe() override {
		addKeyframe(mAnimationTimeProvider.GetTime(), mProvider.get_translation(), mProvider.get_rotation(), mProvider.get_scale());
	}
	
	void UpdateKeyframe() override {
		updateKeyframe(mAnimationTimeProvider.GetTime(), mProvider.get_translation(), mProvider.get_rotation(), mProvider.get_scale());
	}
	void RemoveKeyframe() override {
		removeKeyframe(mAnimationTimeProvider.GetTime());
	}
	
	void Freeze() override {
		mFrozen = true;
	}
	
	void Evaluate() override {
		if (!mFrozen) {
			auto keyframe = evaluate(mAnimationTimeProvider.GetTime());

			if (keyframe.has_value()) {
				auto [t, r, s] = *keyframe;
				
				mProvider.set_translation(t);
				mProvider.set_rotation(r);
				mProvider.set_scale(s);
			}
		} else {
			auto _ = evaluate(mAnimationTimeProvider.GetTime());
		}
	}
	
	void Unfreeze() override {
		mFrozen = false;
	}
	
	bool IsSyncWithProvider() override {
		auto m1 = mProvider.get_matrix();
		auto m2 = evaluate_as_matrix(mAnimationTimeProvider.GetTime());
		
		if (m2.has_value()) {
			return m1 == *m2;
		} else {
			return true;
		}
		
	}
	
	void SyncWithProvider() override {
		auto keyframe = evaluate(mAnimationTimeProvider.GetTime());
		
		if (keyframe.has_value()) {
			auto [t, r, s] = *keyframe;
			
			mProvider.set_translation(t);
			mProvider.set_rotation(r);
			mProvider.set_scale(s);
		}
	}
	
	bool KeyframeExists() override {
		return is_keyframe(mAnimationTimeProvider.GetTime());
	}
	
	float GetPreviousKeyframeTime() override {
		auto keyframe = get_previous_keyframe(mAnimationTimeProvider.GetTime());
		
		if (keyframe.has_value()) {
			return keyframe->time;
		} else {
			return -1;
		}
	}
	
	float GetNextKeyframeTime() override {
		auto keyframe = get_next_keyframe(mAnimationTimeProvider.GetTime());
		
		if (keyframe.has_value()) {
			return keyframe->time;
		} else {
			return -1;
		}
	}

private:
	TransformComponent& mProvider;
	AnimationTimeProvider& mAnimationTimeProvider;
	int mRegistrationId;
	bool mFrozen;
	
	struct Keyframe {
		float time;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
	};
	
	// Add a keyframe to the animation
	void addKeyframe(float time, const glm::vec3& position,
					 const glm::quat& rotation, const glm::vec3& scale) {
		// Check if a keyframe at this time already exists
		if (keyframeExists(time)) {
			// Optionally, you can choose to update the existing keyframe instead
			// For now, we prevent adding duplicate keyframes
			
			updateKeyframe(time, position, rotation, scale);
			return;
		}
		Keyframe keyframe{ time, position, rotation, scale };
		keyframes_.push_back(keyframe);
		// Keep keyframes sorted
		std::sort(keyframes_.begin(), keyframes_.end(),
				  [](const Keyframe& a, const Keyframe& b) {
			return a.time < b.time;
		});
	}
	
	// Update an existing keyframe at a specified time
	void updateKeyframe(float time, const glm::vec3& position,
						const glm::quat& rotation, const glm::vec3& scale) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			it->position = position;
			it->rotation = rotation;
			it->scale = scale;
		} else {
			// Optionally handle the case where the keyframe does not exist
			// For example, you could add the keyframe if it doesn't exist
			addKeyframe(time, position, rotation, scale);
		}
	}
	
	// Remove a keyframe at the specified time
	void removeKeyframe(float time) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			keyframes_.erase(it);
		}
		// Optionally handle the case where the keyframe does not exist
	}
	
	// Evaluate the transformation matrix at a given time
	std::optional<std::tuple<glm::vec3, glm::quat, glm::vec3>> evaluate(float time) const {
		if (keyframes_.empty()) {
			return std::nullopt;
		}
		
		// Clamp time to the bounds of the keyframes
		if (time <= keyframes_.front().time) {
			return decomposeKeyframe(keyframes_.front());
		}
		if (time >= keyframes_.back().time) {
			return decomposeKeyframe(keyframes_.back());
		}
		
		// Find the two keyframes surrounding the given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		const Keyframe& next = *it;
		const Keyframe& prev = *(it - 1);
		
		// Compute interpolation factor
		float factor = (time - prev.time) / (next.time - prev.time);
		
		// Interpolate position
		glm::vec3 position = glm::mix(prev.position, next.position, factor);
		
		// Interpolate rotation
		glm::quat rotation = glm::slerp(prev.rotation, next.rotation, factor);
		
		// Interpolate scale
		glm::vec3 scale = glm::mix(prev.scale, next.scale, factor);
		
		// Compose the transformation matrix
		return std::make_tuple(position, rotation, scale);
	}
	
	std::optional<glm::mat4> evaluate_as_matrix(float time) const {
		if (keyframes_.empty()) {
			return std::nullopt;
		}
		
		// Clamp time to the bounds of the keyframes
		if (time <= keyframes_.front().time) {
			return decomposeKeyframeAsMatrix(keyframes_.front());
		}
		if (time >= keyframes_.back().time) {
			return decomposeKeyframeAsMatrix(keyframes_.back());
		}
		
		// Find the two keyframes surrounding the given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		const Keyframe& next = *it;
		const Keyframe& prev = *(it - 1);
		
		// Compute interpolation factor
		float factor = (time - prev.time) / (next.time - prev.time);
		
		// Interpolate position
		glm::vec3 position = glm::mix(prev.position, next.position, factor);
		
		// Interpolate rotation
		glm::quat rotation = glm::slerp(prev.rotation, next.rotation, factor);
		
		// Interpolate scale
		glm::vec3 scale = glm::mix(prev.scale, next.scale, factor);
		
		// Compose the transformation matrix
		return composeTransform(position, rotation, scale);
	}

	
	// Check if the current time corresponds to an exact keyframe
	bool is_keyframe(float time) const {
		return keyframeExists(time);
	}
	
	// Check if the current time is between two keyframes
	bool is_between_keyframes(float time) const {
		if (keyframes_.size() < 2) {
			return false; // No "between" state if fewer than 2 keyframes
		}
		
		// Find the two keyframes surrounding the given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		// If it's not between any valid keyframes, return false
		if (it == keyframes_.begin() || it == keyframes_.end()) {
			return false;
		}
		
		const Keyframe& prev = *(it - 1);
		const Keyframe& next = *it;
		
		// Return true if time is between two keyframes
		return (time > prev.time && time < next.time);
	}
	
	// New Methods to Get Previous and Next Keyframes
	std::optional<Keyframe> get_previous_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		// Find the first keyframe where time is greater than or equal to the given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		if (it == keyframes_.begin()) return std::nullopt; // No previous keyframe
		--it;
		return *it;
	}

	std::optional<Keyframe> get_next_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		// Find the first keyframe with time greater than the given time
		auto it = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const float t, const Keyframe& kf) {
			return t < kf.time; // We are looking for the first keyframe where time is greater
		});
		if (it == keyframes_.end()) return std::nullopt; // No next keyframe
		return *it;
	}

	
private:
	std::vector<Keyframe> keyframes_;
	
	// Helper to compose a transformation matrix from a keyframe
	std::tuple<glm::vec3, glm::quat, glm::vec3> decomposeKeyframe(const Keyframe& kf) const {
		return std::make_tuple(kf.position, kf.rotation, kf.scale);
	}
	
	glm::mat4 decomposeKeyframeAsMatrix(const Keyframe& kf) const {
		return composeTransform(kf.position, kf.rotation, kf.scale);
	}

	
	// Helper to compose a transformation matrix from components
	glm::mat4 composeTransform(const glm::vec3& position,
							   const glm::quat& rotation,
							   const glm::vec3& scale) const {
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 rotationMat = glm::toMat4(rotation);
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
		return translation * rotationMat * scaleMat;
	}
	
	// Helper to check if a keyframe exists at the given time
	bool keyframeExists(float time) const {
		return std::any_of(keyframes_.begin(), keyframes_.end(),
						   [time](const Keyframe& kf) { return kf.time == time; });
	}
	
	// Helper to find a keyframe at the given time
	typename std::vector<Keyframe>::iterator findKeyframe(float time) {
		return std::find_if(keyframes_.begin(), keyframes_.end(),
							[time](const Keyframe& kf) { return kf.time == time; });
	}
	
	typename std::vector<Keyframe>::const_iterator findKeyframe(float time) const {
		return std::find_if(keyframes_.cbegin(), keyframes_.cend(),
							[time](const Keyframe& kf) { return kf.time == time; });
	}
};
