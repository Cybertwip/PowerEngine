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
	: mAnimationPdo(std::move(animationPdo)), mSkeleton(*mAnimationPdo->mSkeleton), mCurrentTime(0.0f) {
		for (auto& animation : mAnimationPdo->mAnimationData) {
			mAnimationData.push_back(*animation);
		}
		
		// Initialize the pose buffers
		size_t numBones = mSkeleton.get().num_bones();
		mModelPose.resize(numBones);
		
		const Animation& animation = mAnimationData[0].get();

		evaluate_animation(animation, mCurrentTime); // initial evaluation
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
	}
	
	// Remove a keyframe at the specified time
	void removeKeyframe(float time) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			keyframes_.erase(it);
		}
		// Optionally handle the case where the keyframe does not exist
	}
	
	// Evaluate the playback state at a given time
	PlaybackState evaluate(float time) const {
		if (keyframes_.empty()) {
			return PlaybackState::Pause; // Default state if no keyframes
		}
		
		// Clamp time to the bounds of the keyframes
		if (time <= keyframes_.front().time) {
			return keyframes_.front().getPlaybackState();
		}
		if (time >= keyframes_.back().time) {
			return keyframes_.back().getPlaybackState();
		}
		
		// Find the two keyframes surrounding the given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const Keyframe& kf, float t) {
			return kf.time < t;
		});
		
		const Keyframe& next = *it;
		const Keyframe& prev = *(it - 1);
		
		// Compute interpolation factor (if needed for continuous properties)
		float factor = (time - prev.time) / (next.time - prev.time);
		
		// For discrete properties like PlaybackState, choose based on factor
		return (factor < 0.5f) ? prev.getPlaybackState() : next.getPlaybackState();
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
	void update(float deltaTime) {
		if (mAnimationData.empty()) {
			return; // No animations to process
		}
		
		const Animation& animation = mAnimationData[0].get();
		float duration = static_cast<float>(animation.get_duration());
		
		// Evaluate the playback state at the current time
		PlaybackState currentState = evaluate(mCurrentTime);
		
		// If paused, do not advance time
		if (currentState == PlaybackState::Pause) {
			// Optionally, you can still update the skeleton pose if needed
			
			if (keyframes_.empty()) {
				Skeleton& skeleton = mSkeleton.get();
				skeleton.compute_offsets({});
			} else {
				apply_pose_to_skeleton();
			}

			return;
		}
		
		// Determine playback direction
		bool reverse = false;
		if (auto modifierOpt = get_current_playback_modifier(mCurrentTime)) {
			reverse = (*modifierOpt == PlaybackModifier::Reverse);
		}
		
		// Update current time based on playback direction
		mCurrentTime += deltaTime * (reverse ? -1.0f : 1.0f);
		
		// Wrap current time within the duration
		if (mCurrentTime < 0.0f) mCurrentTime += duration;
		if (mCurrentTime > duration) mCurrentTime = std::fmod(mCurrentTime, duration);
		
		// Evaluate the animation at the current time
		evaluate_animation(animation, mCurrentTime);
		
		// Update the skeleton with the new transforms
		apply_pose_to_skeleton();
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
		update(1);
		
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


	std::unique_ptr<SkinnedAnimationPdo> mAnimationPdo;
	
	std::reference_wrapper<Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<Animation>> mAnimationData;
	
	float mCurrentTime; // Current animation time
	
	// Buffers to store poses
	std::vector<glm::mat4> mModelPose;
	
	// Keyframes for playback control
	std::vector<Keyframe> keyframes_;
};
