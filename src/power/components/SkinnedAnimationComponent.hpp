#pragma once

#include "animation/Animation.hpp"
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

class SkinnedAnimationComponent {
public:
	enum class PlaybackState {
		Play,
		Pause
	};
	
	enum class PlaybackModifier {
		Forward,
		Reverse
	};
	
	enum class PlaybackTrigger {
		None,
		Rewind,
		FastForward
	};
	
	struct Keyframe {
		float time; // Keyframe time
	private:
		PlaybackState mPlaybackState;
		PlaybackModifier mPlaybackModifier;
		PlaybackTrigger mPlaybackTrigger;
		
	public:
		// Constructors
		Keyframe(float t, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger)
		: time(t), mPlaybackState(state), mPlaybackModifier(modifier), mPlaybackTrigger(trigger) {
			// Setting trigger sets state to Pause
			if (trigger == PlaybackTrigger::Rewind || trigger == PlaybackTrigger::FastForward) {
				mPlaybackState = PlaybackState::Pause;
			}
		}
		
		// Getter for PlaybackState
		PlaybackState getPlaybackState() const {
			return mPlaybackState;
		}
		
		// Setter for PlaybackState
		void setPlaybackState(PlaybackState state) {
			mPlaybackState = state;
		}
		
		// Getter for PlaybackModifier
		PlaybackModifier getPlaybackModifier() const {
			return mPlaybackModifier;
		}
		
		// Setter for PlaybackModifier
		void setPlaybackModifier(PlaybackModifier modifier) {
			mPlaybackModifier = modifier;
		}
		
		// Getter for PlaybackTrigger
		PlaybackTrigger getPlaybackTrigger() const {
			return mPlaybackTrigger;
		}
		
		// Setter for PlaybackTrigger
		void setPlaybackTrigger(PlaybackTrigger trigger) {
			mPlaybackTrigger = trigger;
			// Setting the trigger should also set PlaybackState to Pause
			if (mPlaybackTrigger != PlaybackTrigger::None) {
				mPlaybackState = PlaybackState::Pause;
			}
		}
		
		// Overloaded == operator for Keyframe
		bool operator==(const Keyframe& rhs) {
			return time == rhs.time &&
			getPlaybackState() == rhs.getPlaybackState() &&
			getPlaybackModifier() == rhs.getPlaybackModifier() &&
			getPlaybackTrigger() == rhs.getPlaybackTrigger();
		}
		
		// Overloaded != operator for Keyframe
		bool operator!=(const Keyframe& rhs) {
			return !(*this == rhs);
		}
	};
	
	struct BoneCPU {
		float transform[4][4] =
		{
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
	};
	
	struct SkinnedAnimationPdo {
		SkinnedAnimationPdo(std::unique_ptr<Skeleton> skeleton) : mSkeleton(std::move(skeleton)) {}
		
		std::unique_ptr<Skeleton> mSkeleton;
		std::vector<std::unique_ptr<Animation>> mAnimationData;
	};
	
public:
	SkinnedAnimationComponent(std::unique_ptr<SkinnedAnimationPdo> animationPdo)
	: mAnimationPdo(std::move(animationPdo))
	, mSkeleton(*mAnimationPdo->mSkeleton)
	, mAnimationOffset(0.0f) {
		for (auto& animation : mAnimationPdo->mAnimationData) {
			mAnimationData.push_back(*animation);
		}
		
		// Initialize the pose buffers
		size_t numBones = mSkeleton.get().num_bones();
		mModelPose.resize(numBones);
		
		std::fill(mModelPose.begin(), mModelPose.end(), glm::identity<glm::mat4>());
		std::fill(mEmptyPose.begin(), mEmptyPose.end(), glm::identity<glm::mat4>());

		apply_pose_to_skeleton();
	}
	
	// Add a keyframe to the animation
	void addKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger) {
		// Check if a keyframe at this time already exists
		if (keyframeExists(time)) {
			// Update the existing keyframe
			updateKeyframe(time, state, modifier, trigger);
			return;
		}
		Keyframe keyframe(time, state, modifier, trigger);
		keyframes_.push_back(keyframe);
		// Keep keyframes sorted
		std::sort(keyframes_.begin(), keyframes_.end(),
				  [](const Keyframe& a, const Keyframe& b) {
			return a.time < b.time;
		});
		
		updateAnimationOffset();
	}
	
	// Update an existing keyframe at a specified time
	void updateKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			it->setPlaybackState(state);
			it->setPlaybackModifier(modifier);
			it->setPlaybackTrigger(trigger);
		} else {
			// If the keyframe does not exist, add it
			addKeyframe(time, state, modifier, trigger);
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
	std::optional<Keyframe> evaluate(float time) {
		if (keyframes_.empty()) {
			return std::nullopt; // Default state if no keyframes
		}
		
		// If time is before the first keyframe
		if (time < keyframes_.front().time) {
			// Set the model pose to the first keyframe's pose
			const Animation& animation = mAnimationData[0].get();

			evaluate_animation(animation, keyframes_.front().time);
			
			return std::nullopt;
		}
		
		// If time is after the last keyframe
		if (time > keyframes_.back().time) {
			// Set the model pose to the last keyframe's pose
			
			const Animation& animation = mAnimationData[0].get();

			evaluate_animation(animation, keyframes_.back().time);
			
			return keyframes_.back();
		}
		
		// Find the first keyframe with time >= given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		if (it != keyframes_.end()) {
			if (it->time == time) {
				return *it;
			} else if (it != keyframes_.begin()) {
				// No exact match; use the closest keyframe to the left
				const Keyframe& current = *(it - 1);
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
	std::optional<Keyframe> get_previous_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		if (it == keyframes_.begin()) return std::nullopt; // No previous keyframe
		--it;
		return *it;
	}
	
	// Get the next keyframe after the given time
	std::optional<Keyframe> get_next_keyframe(float time) const {
		if (keyframes_.empty()) return std::nullopt;
		
		auto it = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](float t, const Keyframe& kf) {
			return t < kf.time;
		});
		if (it == keyframes_.end()) return std::nullopt; // No next keyframe
		return *it;
	}
	
	// Update the animation based on the keyframes
	void evaluate_time(float time) {
		if (mAnimationData.empty()) {
			return; // No animations to process
		}
		
		const Animation& animation = mAnimationData[0].get();
		
		float duration = static_cast<float>(animation.get_duration());
						
		// Evaluate the playback state at the current time
		auto keyframe = evaluate(time);
		
		PlaybackState currentState = keyframe == std::nullopt ? PlaybackState::Pause : keyframe->getPlaybackState();
		
		PlaybackModifier currentModifier = keyframe == std::nullopt ? PlaybackModifier::Forward : keyframe->getPlaybackModifier();

		// Determine playback direction
		bool reverse = currentModifier == PlaybackModifier::Reverse;
		
		float animationTime = time;
		
		// Update current time based on playback direction
		// If paused, do not advance time
		if (currentState == PlaybackState::Play) {
			animationTime += (time - animationTime) * (reverse ? -1.0f : 1.0f);
			if (keyframe.has_value()) {
				auto previousPauseStateKeyFrame = getPreviousPauseStateKeyframe(keyframe->time);

				if (previousPauseStateKeyFrame.has_value()) {
					animationTime = adjust_wrapped_time(animationTime, keyframe->time, keyframe->time + duration, mAnimationOffset + previousPauseStateKeyFrame->time, duration);
				} else {
					animationTime = adjust_wrapped_time(animationTime, mAnimationOffset, keyframe->time + duration, keyframe->time, duration);
				}
			}
		} else {
			if (keyframe.has_value()) {
				
				auto previousPlayStateKeyFrame = getPreviousPlayStateKeyframe(keyframe->time);

				auto previousPauseStateKeyFrame = getFirstButPreviousPauseStateKeyframe(keyframe->time);
				
				if (previousPauseStateKeyFrame.has_value() && previousPlayStateKeyFrame.has_value()) {
					animationTime = adjust_wrapped_time(previousPauseStateKeyFrame->time, previousPlayStateKeyFrame->time, previousPauseStateKeyFrame->time + duration, previousPauseStateKeyFrame->time, duration);
				} else if(previousPlayStateKeyFrame.has_value()) {
					animationTime = adjust_wrapped_time(keyframe->time, previousPlayStateKeyFrame->time, keyframe->time + duration, mAnimationOffset, duration);
				} else if(previousPauseStateKeyFrame.has_value()) {
					animationTime = adjust_wrapped_time(previousPauseStateKeyFrame->time, previousPauseStateKeyFrame->time, previousPauseStateKeyFrame->time + duration, mAnimationOffset, duration);
				} else {
					if (keyframes_.empty()) {
						animationTime = 0.0f;
					} else {
						animationTime = adjust_wrapped_time(keyframe->time, keyframe->time, keyframe->time + duration, mAnimationOffset, duration);
					}
				}
			} else {
				animationTime = 0.0f;
			}
		}
		
		// Evaluate the animation at the current time
		evaluate_animation(animation, animationTime);
		
		// Update the skeleton with the new transforms
		apply_pose_to_skeleton();
	}
	
	
	int adjust_wrapped_time(int currentTime, int start_time, int end_time, int offset, int duration) {
		int wrapped_time = currentTime;
		if (wrapped_time != -1) {
			if (offset < 0 && wrapped_time + offset < start_time + offset) {
				wrapped_time = duration - std::fabs(std::fmod(wrapped_time - start_time, duration));
			} else {
				wrapped_time = std::fmod(wrapped_time - start_time, duration);
			}
			
			if (wrapped_time > end_time) {
				wrapped_time = start_time;
			}
			
		}
		return wrapped_time;
	}
	
	int get_animation_duration() {
		if (mAnimationData.empty()) {
			return 0;
		}
		
		const Animation& animation = mAnimationData[0].get();
		
		return animation.get_duration();
	}
	
	// Retrieve the bones for rendering
	std::vector<BoneCPU> get_bones() {
		// Ensure we have a valid number of bones
		size_t numBones = mSkeleton.get().num_bones();
		
		std::vector<BoneCPU> bonesCPU(numBones);
		
		for (size_t i = 0; i < numBones; ++i) {
			// Get the bone transform as a glm::mat4
			glm::mat4 boneTransform = mSkeleton.get().get_bone(i).transform;
			
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
	
	std::vector<BoneCPU> get_bones_at_time(int time) {
		if (mAnimationData.empty()) {
			return; // No animations to process
		}
		
		// For simplicity, use the first animation in the list
		const Animation& animation = mAnimationData[0].get();
		int duration = animation.get_duration();
		
		time = fmax(0, fmod(duration + time, duration));
		
		// Evaluate the animation at the current time
		auto model = evaluate_animation_once(animation, time);
		
		// Update the skeleton with the new transforms
		apply_pose_to_skeleton(model);
		
		// Ensure we have a valid number of bones
		size_t numBones = mSkeleton.get().num_bones();
		
		std::vector<BoneCPU> bonesCPU(numBones);
		
		for (size_t i = 0; i < numBones; ++i) {
			// Get the bone transform as a glm::mat4
			glm::mat4 boneTransform = mSkeleton.get().get_bone(i).transform;
			
			// Reference to the BoneCPU structure
			BoneCPU& boneCPU = bonesCPU[i];
			
			// Copy each element from glm::mat4 to the BoneCPU's transform array
			for (int row = 0; row < 4; ++row) {
				for (int col = 0; col < 4; ++col) {
					boneCPU.transform[row][col] = boneTransform[row][col];
				}
			}
		}
		
		apply_pose_to_skeleton(); // return default pose

		return bonesCPU;
		
	}
	
	
	void set_pdo(std::unique_ptr<SkinnedAnimationPdo> animationPdo){
		mAnimationData.clear();
		
		// skeleton does not change but must match this animation.
		animationPdo->mSkeleton = std::move(mAnimationPdo->mSkeleton);
		
		// do skeleton matching (index and bone naming)
		
		//@TODO
		
		//
		
		
		mAnimationPdo = std::move(animationPdo);
		
		
		for (auto& animation : mAnimationPdo->mAnimationData) {
			mAnimationData.push_back(*animation);
		}
		
	}
	
	Keyframe get_keyframe(float time) const {
		return *findKeyframe(time);
	}

private:
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
		Skeleton& skeleton = mSkeleton.get();
		skeleton.compute_offsets(mModelPose);
	}
	
	void apply_pose_to_skeleton(std::vector<glm::mat4> modelPose) {
		Skeleton& skeleton = mSkeleton.get();
		skeleton.compute_offsets(modelPose);
	}
	
	void updateAnimationOffset() {
		const Animation& animation = mAnimationData[0].get();
		float duration = static_cast<float>(animation.get_duration());
		
		// Reset offset to 0.0f by default
		mAnimationOffset = 0.0f;
		
		// Find the first keyframe
		for (const auto& kf : keyframes_) {
			mAnimationOffset = kf.time;
			break;
		}
	}
	
	std::optional<Keyframe> getPreviousPlayStateKeyframe(float fromTime) {
		// Iterate through keyframes in reverse to find the first one before fromTime with PlaybackState::Play
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Play) {
				return kf;
			}
		}
		
		return std::nullopt;
	}
	
	std::optional<Keyframe> getFirstButPreviousPauseStateKeyframe(float fromTime) {
		// Iterate through keyframes in reverse to find the first one before fromTime with PlaybackState::Play
		
		std::optional<Keyframe> keyframe;
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Play) {
				return keyframe;
			} else if (kf.time < fromTime) {
				keyframe = kf;
			}
		}
		
		return std::nullopt;
	}
	
	std::optional<Keyframe> getPreviousPauseStateKeyframe(float fromTime) {
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			const Keyframe& kf = *it;
			if (kf.time < fromTime && kf.getPlaybackState() == PlaybackState::Pause) {
				return kf;
			}
		}
		
		return std::nullopt;
	}

	std::unique_ptr<SkinnedAnimationPdo> mAnimationPdo;
	
	std::reference_wrapper<Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<Animation>> mAnimationData;
	
	float mAnimationOffset; // Animation offset time
	
	// Buffers to store poses
	std::vector<glm::mat4> mModelPose;
	std::vector<glm::mat4> mEmptyPose;

	// Keyframes for playback control
	std::vector<Keyframe> keyframes_;
};
