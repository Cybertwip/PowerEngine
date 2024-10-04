#pragma once

#include "AnimationComponent.hpp"
#include "PlaybackComponent.hpp"

#include "animation/Animation.hpp"
#include "animation/AnimationTimeProvider.hpp"
#include "animation/Transform.hpp"
#include "animation/Skeleton.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>
#include <optional>

class SkinnedAnimationComponent : public AnimationComponent {
public:
	struct BoneCPU {
		float transform[4][4] =
		{
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
	};
	
public:
	SkinnedAnimationComponent(PlaybackComponent& provider, AnimationTimeProvider& animationTimeProvider)
	: mProvider(provider), mAnimationTimeProvider(animationTimeProvider), mRegistrationId(-1), mFrozen(false)
	, mAnimationOffset(0.0f) {
		// Initialize the pose buffers
		size_t numBones = provider.get_skeleton().num_bones();
		mModelPose.resize(numBones);
		
		std::fill(mModelPose.begin(), mModelPose.end(), glm::identity<glm::mat4>());
		std::fill(mEmptyPose.begin(), mEmptyPose.end(), glm::identity<glm::mat4>());
		
		apply_pose_to_skeleton();
	}
	
	
	void TriggerRegistration() override {
		if(mRegistrationId != -1) {
			mProvider
				.unregister_on_playback_changed_callback(mRegistrationId);
			
			mRegistrationId = -1;
		}
		
		mRegistrationId = mProvider.register_on_playback_changed_callback([this](const PlaybackComponent& playback) {
			
			if (mFrozen) {
				addKeyframe(mAnimationTimeProvider.GetTime(), playback.getPlaybackState(), playback.getPlaybackModifier(), playback.getPlaybackTrigger(), playback.getPlaybackData());
			}
		});
	}
	
	void AddKeyframe() override {
		addKeyframe(mAnimationTimeProvider.GetTime(), mProvider.getPlaybackState(), mProvider.getPlaybackModifier(), mProvider.getPlaybackTrigger(), mProvider.getPlaybackData());
	}
	
	void UpdateKeyframe() override {
		updateKeyframe(mAnimationTimeProvider.GetTime(), mProvider.getPlaybackState(), mProvider.getPlaybackModifier(), mProvider.getPlaybackTrigger(),
		    mProvider.getPlaybackData());
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
				mProvider.setPlaybackState(keyframe->getPlaybackState());
				mProvider.setPlaybackModifier(keyframe->getPlaybackModifier());
				mProvider.setPlaybackTrigger(keyframe->getPlaybackTrigger());
				mProvider.setPlaybackData(keyframe->getPlaybackData());
			}

		}
	}
	
	void Unfreeze() override {
		mFrozen = false;
	}
	
	bool IsSyncWithProvider() override {
		auto m1 = const_cast<PlaybackComponent::Keyframe&>(mProvider.get_state());
		m1.time = mAnimationTimeProvider.GetTime();
		
		auto m2 = evaluate_keyframe(mAnimationTimeProvider.GetTime());
				
		if (m2.has_value()) {
			return m1 == *m2;
		} else {
			return true;
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
	PlaybackComponent& mProvider;
	AnimationTimeProvider& mAnimationTimeProvider;
	int mRegistrationId;
	bool mFrozen;
	
public:
	// Add a keyframe to the animation
	void addKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData) {
		// Check if a keyframe at this time already exists
		if (keyframeExists(time)) {
			// Update the existing keyframe
			updateKeyframe(time, state, modifier, trigger, playbackData);
			return;
		}
		PlaybackComponent::Keyframe keyframe(time, state, modifier, trigger, playbackData);
		keyframes_.push_back(keyframe);
		// Keep keyframes sorted
		std::sort(keyframes_.begin(), keyframes_.end(),
				  [](const PlaybackComponent::Keyframe& a, const PlaybackComponent::Keyframe& b) {
			return a.time < b.time;
		});
		
		updateAnimationOffset();
	}
	
	// Update an existing keyframe at a specified time
	void updateKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			it->setPlaybackState(state);
			it->setPlaybackModifier(modifier);
			it->setPlaybackTrigger(trigger);
			it->setPlaybackData(playbackData);
		} else {
			// If the keyframe does not exist, add it
			addKeyframe(time, state, modifier, trigger, playbackData);
		}
		
		updateAnimationOffset();
	}
	
	// Remove a keyframe at the specified time
	void removeKeyframe(float time) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			keyframes_.erase(it);
		}
		// Optionally handle the case where the keyframe does not exist
		
		updateAnimationOffset();
	}
	
	// Evaluate the playback state at a given time
	std::optional<PlaybackComponent::Keyframe> evaluate_keyframe(float time) {
		if (keyframes_.empty()) {
			return std::nullopt; // Default state if no keyframes
		}
		
		// If time is before the first keyframe
		if (time < keyframes_.front().time) {
			// Set the model pose to the first keyframe's pose
			evaluate_animation(*keyframes_.front().getPlaybackData()->mAnimation, keyframes_.front().time);
			
			return std::nullopt;
		}
		
		// If time is after the last keyframe
		if (time > keyframes_.back().time) {
			// Set the model pose to the last keyframe's pose
			evaluate_animation(*keyframes_.back().getPlaybackData()->mAnimation, keyframes_.back().time);
			
			return keyframes_.back();
		}
		
		// Find the first keyframe with time >= given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const PlaybackComponent::Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		if (it != keyframes_.end()) {
			if (it->time == time) {
				return *it;
			} else if (it != keyframes_.begin()) {
				// No exact match; use the closest keyframe to the left
				const PlaybackComponent::Keyframe& current = *(it - 1);
				return current;
			}
		}
		
		// Fallback (should not reach here if bounds are respected)
		return std::nullopt;
	}
	
	// Check if the current time corresponds to an exact keyframe
	bool is_keyframe(float time) const {
		return keyframeExists(time);
	}
	
	// Get the previous keyframe before the given time
	std::optional<PlaybackComponent::Keyframe> get_previous_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const PlaybackComponent::Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		if (it == keyframes_.begin()) return std::nullopt; // No previous keyframe
		--it;
		return *it;
	}
	
	// Get the next keyframe after the given time
	std::optional<PlaybackComponent::Keyframe> get_next_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		auto it = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](float t, const PlaybackComponent::Keyframe& kf) {
			return t < kf.time;
		});
		if (it == keyframes_.end()) return std::nullopt; // No next keyframe
		return *it;
	}
	
	float getAdjustedAnimationTime(float currentTime, float duration) {
		float accumulatedPlayTime = 0.0f;
		
		// If there are no keyframes, the animation remains at the initial offset
		if (keyframes_.empty()) {
			return 0.0f;
		}
		
		// Initialize lastState and lastStateChangeTime based on the first keyframe
		float lastStateChangeTime = mAnimationOffset;
		PlaybackState lastState = keyframes_[0].getPlaybackState();
		
		size_t keyframeIndex = 0;
		
		// If the first keyframe is after currentTime, the animation hasn't started yet
		if (keyframes_[0].time > currentTime) {
			return 0.0f;
		}
		
		// Process keyframes
		for (; keyframeIndex < keyframes_.size(); ++keyframeIndex) {
			const auto& keyframe = keyframes_[keyframeIndex];
			if (keyframe.time > currentTime) {
				break;
			}
			
			// Handle state changes
			PlaybackState currentState = keyframe.getPlaybackState();
			
			if (currentState != lastState) {
				if (lastState == PlaybackState::Play) {
					// Accumulate the duration from the last play state to the current keyframe time
					accumulatedPlayTime += keyframe.time - lastStateChangeTime;
				}
				lastState = currentState;
				lastStateChangeTime = keyframe.time;
			}
		}
		
		// After processing keyframes up to currentTime
		if (lastState == PlaybackState::Play) {
			// Accumulate time from last state change to current time
			accumulatedPlayTime += currentTime - lastStateChangeTime;
		}
		
		// Wrap the accumulated play time around the animation duration
		float adjustedTime = fmod(accumulatedPlayTime, duration);
		
		return adjustedTime;
	}
	
	std::optional<PlaybackComponent::Keyframe>  evaluate(float time) {
		auto keyframe = evaluate_keyframe(time);

		std::optional<std::reference_wrapper<Animation>> animation;

		if (keyframe.has_value()) {
			mAnimationOffset = keyframe->time;
			animation = *keyframe->getPlaybackData()->mAnimation;
		} else {
			return std::nullopt;
		}
		
		float duration = static_cast<float>(animation->get().get_duration());

		PlaybackModifier currentModifier = PlaybackModifier::Forward;
		
		if (keyframe.has_value()) {
			currentModifier = keyframe->getPlaybackModifier();
		}
		
		bool reverse = currentModifier == PlaybackModifier::Reverse;
		float animationTime = 0.0f;
		
		// Calculate adjusted animation time
		animationTime = getAdjustedAnimationTime(time, duration);
		
		// Handle reverse playback
		if (reverse) {
			animationTime = fmod(duration - animationTime, duration);
		}
		
		// Evaluate the animation at the adjusted time
		evaluate_animation(animation->get(), animationTime);
		
		// Update the skeleton with the new transforms
		apply_pose_to_skeleton();
		
		return keyframe;
	}
	
	// Retrieve the bones for rendering
	std::vector<BoneCPU> get_bones() {
		// Ensure we have a valid number of bones
		size_t numBones = mProvider.get_skeleton().num_bones();
		
		std::vector<BoneCPU> bonesCPU(numBones);
		
		for (size_t i = 0; i < numBones; ++i) {
			// Get the bone transform as a glm::mat4
			glm::mat4 boneTransform = mProvider.get_skeleton().get_bone(i).transform;
			
			// Reference to the BoneCPU structure
			BoneCPU& boneCPU = bonesCPU[i];
			
			// Copy each element from glm::mat4 to the BoneCPU's transform array
			for (int row = 0; row < 4; ++row) {
				for (int col = 0; col < 4; ++col) {
					boneCPU.transform[row][col] = boneTransform[row][col];
				}
			}
		}
		return bonesCPU;
	}
	
	std::optional<PlaybackComponent::Keyframe> get_keyframe(float time) const {
		auto it = findKeyframe(time);
		
		if (it != keyframes_.end()) {
			return *it;
		} else {
			return std::nullopt;
		}
	}

private:
	// Helper to check if a keyframe exists at the given time
	bool keyframeExists(float time) const {
		return std::any_of(keyframes_.begin(), keyframes_.end(),
						   [time](const PlaybackComponent::Keyframe& kf) { return kf.time == time; });
	}
	
	// Helper to find a keyframe at the given time
	typename std::vector<PlaybackComponent::Keyframe>::iterator findKeyframe(float time) {
		return std::find_if(keyframes_.begin(), keyframes_.end(),
							[time](const PlaybackComponent::Keyframe& kf) { return kf.time == time; });
	}
	
	typename std::vector<PlaybackComponent::Keyframe>::const_iterator findKeyframe(float time) const {
		return std::find_if(keyframes_.cbegin(), keyframes_.cend(),
							[time](const PlaybackComponent::Keyframe& kf) { return kf.time == time; });
	}
	
	// Helper to get the current playback modifier at a given time
	std::optional<PlaybackModifier> get_current_playback_modifier(float time) const {
		if (keyframes_.empty()) {
			return std::nullopt;
		}
		
		// Find the keyframe at or before the current time
		auto prevKeyframeOpt = get_previous_keyframe(time);
		if (prevKeyframeOpt) {
			return prevKeyframeOpt->getPlaybackModifier();
		}
		return std::nullopt;
	}
	
	std::vector<glm::mat4> evaluate_animation_once(const Animation& animation, float time) {
		return animation.evaluate(time);
	}
	
	void evaluate_animation(const Animation& animation, float time) {
		mModelPose = animation.evaluate(time);
	}
	
	void apply_pose_to_skeleton() {
		Skeleton& skeleton = mProvider.get_skeleton();
		skeleton.compute_offsets(mModelPose);
	}
	
	void apply_pose_to_skeleton(std::vector<glm::mat4> modelPose) {
		Skeleton& skeleton = mProvider.get_skeleton();
		skeleton.compute_offsets(modelPose);
	}
	
	void updateAnimationOffset() {
		// Reset offset to 0.0f by default
		mAnimationOffset = 0.0f;
		
		// Find the first keyframe
		for (const auto& kf : keyframes_) {
			mAnimationOffset = kf.time;
			break;
		}
	}
	
	std::optional<PlaybackComponent::Keyframe> getPreviousPlayStateKeyframe(float fromTime) {
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const PlaybackComponent::Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Play) {
				return kf;
			}
		}
		
		return std::nullopt;
	}
	
	std::optional<PlaybackComponent::Keyframe> getPreviousPauseStateKeyframe(float fromTime) {
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const PlaybackComponent::Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Pause) {
				return kf;
			}
		}
		
		return std::nullopt;
	}
	
	std::optional<PlaybackComponent::Keyframe> getFirstButPreviousPauseStateKeyframe(float fromTime) {
		std::optional<PlaybackComponent::Keyframe> keyframe;
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const PlaybackComponent::Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Play) {
				return keyframe;
			} else if (kf.time < fromTime) {
				keyframe = kf;
			}
		}
		
		return std::nullopt;
	}
	
	std::optional<PlaybackComponent::Keyframe> getFirstButPreviousPlayStateKeyframe(float fromTime) {
		std::optional<PlaybackComponent::Keyframe> keyframe;
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const PlaybackComponent::Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Pause) {
				return keyframe;
			} else if (kf.time < fromTime) {
				keyframe = kf;
			}
		}
		
		return std::nullopt;
	}
	
	float mAnimationOffset; // Animation offset time
	
	// Buffers to store poses
	std::vector<glm::mat4> mModelPose;
	std::vector<glm::mat4> mEmptyPose;

	// Keyframes for playback control
	std::vector<PlaybackComponent::Keyframe> keyframes_;
};
