#pragma once

#include "AnimationComponent.hpp"
#include "PlaybackComponent.hpp"
#include "SkeletonComponent.hpp"

#include "animation/Animation.hpp"
#include "animation/AnimationTimeProvider.hpp"
#include "animation/Transform.hpp"
#include "animation/Skeleton.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>
#include <optional>
#include <functional>

class SkinnedPlaybackComponent : public AnimationComponent {
private:
	SkinnedPlaybackComponent();
	
public:
	SkinnedPlaybackComponent(PlaybackComponent& provider, SkeletonComponent& skeletonComponent, AnimationTimeProvider& animationTimeProvider)
	: mProvider(provider), mSkeletonComponent(skeletonComponent), mAnimationTimeProvider(animationTimeProvider), mRegistrationId(-1), mFrozen(false)
	, mAnimationOffset(0.0f) {
		// Initializing pose buffers
		size_t numBones = mSkeletonComponent.get_skeleton().num_bones();
		mModelPose.resize(numBones);
		mDefaultPose.resize(numBones);
		
		for (size_t i = 0; i < numBones; ++i) {
			auto defaultTransform = glm::identity<glm::mat4>();
			
			mModelPose[i] = defaultTransform;
			mDefaultPose[i] = defaultTransform;
		}
		
		apply_pose_to_skeleton();
	}
	
	~SkinnedPlaybackComponent() = default;
	
	void TriggerRegistration() override {
		if(mRegistrationId != -1) {
			mProvider.unregister_on_playback_changed_callback(mRegistrationId);
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
			auto keyframe = evaluate_keyframe(mAnimationTimeProvider.GetTime());
			
			if (keyframe) {
				mProvider.setPlaybackState(keyframe->getPlaybackState());
				mProvider.setPlaybackModifier(keyframe->getPlaybackModifier());
				mProvider.setPlaybackTrigger(keyframe->getPlaybackTrigger());
				mProvider.setPlaybackData(keyframe->getPlaybackData());
			} else {
				evaluate_provider(mAnimationTimeProvider.GetTime(), mProvider.getPlaybackModifier());
			}
		} else {
			auto _ = evaluate_keyframe(mAnimationTimeProvider.GetTime());
		}
	}
	
	void Unfreeze() override {
		mFrozen = false;
	}
	
	bool IsSyncWithProvider() override {
		auto m1 = mProvider.get_state();
		m1->time = mAnimationTimeProvider.GetTime();
		
		auto m2 = evaluate_keyframe(mAnimationTimeProvider.GetTime());
		
		if (m2) {
			return m1 == m2;
		} else {
			return true;
		}
	}
	
	void SyncWithProvider() override {
		auto keyframe = evaluate_keyframe(mAnimationTimeProvider.GetTime());
		
		if (keyframe) {
			mProvider.setPlaybackState(keyframe->getPlaybackState());
			mProvider.setPlaybackModifier(keyframe->getPlaybackModifier());
			mProvider.setPlaybackTrigger(keyframe->getPlaybackTrigger());
			mProvider.setPlaybackData(keyframe->getPlaybackData());
		}
	}
	
	bool KeyframeExists() override {
		return is_keyframe(mAnimationTimeProvider.GetTime());
	}
	
	float GetPreviousKeyframeTime() override {
		auto keyframe = get_previous_keyframe(mAnimationTimeProvider.GetTime());
		
		if (keyframe) {
			return keyframe->time;
		} else {
			return -1;
		}
	}
	
	float GetNextKeyframeTime() override {
		auto keyframe = get_next_keyframe(mAnimationTimeProvider.GetTime());
		
		if (keyframe) {
			return keyframe->time;
		} else {
			return -1;
		}
	}
	
	void reset_pose() {
		apply_pose_to_skeleton(mDefaultPose);
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> evaluate_keyframe(float time) {
		if (keyframes_.empty()) {
			return nullptr;
		}
		
		// Finding the next keyframe after the current time
		auto nextKeyframeIt = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
											   [](float t, const std::shared_ptr<PlaybackComponent::Keyframe>& kf) {
			return t < kf->time;
		});
		
		if (nextKeyframeIt == keyframes_.end()) {
			// If there is no next keyframe, use the last keyframe
			return evaluate_keyframe(keyframes_.back(), time);
		}
		
		if (nextKeyframeIt == keyframes_.begin()) {
			// If the current time is before the first keyframe
			return evaluate_keyframe(*nextKeyframeIt, time);
		}
		
		// Getting the previous keyframe (current keyframe)
		auto currentKeyframeIt = nextKeyframeIt - 1;
		
		// Calculating interpolation factor
		float segmentDuration = (*nextKeyframeIt)->time - (*currentKeyframeIt)->time;
		float t = (time - (*currentKeyframeIt)->time) / segmentDuration;
		
		t = glm::clamp(t, 0.0f, 1.0f);
		
		// Handling playback modifiers (e.g., reverse)
		PlaybackModifier currentModifier = (*currentKeyframeIt)->getPlaybackModifier();
		bool reverse = (currentModifier == PlaybackModifier::Reverse);
		if (reverse) {
			t = 1.0f - t;
		}
		
		// Blending poses from current and next keyframes on a per-bone basis
		blend_keyframes(*currentKeyframeIt, *nextKeyframeIt, time, t);
		
		return *currentKeyframeIt;
	}
	
	void evaluate_provider(float time, PlaybackModifier modifier) {
		Animation& animation = mProvider.get_animation();
		
		if (animation.get_duration() > 0) {
			float duration = static_cast<float>(animation.get_duration());
			bool reverse = modifier == PlaybackModifier::Reverse;
			float animationTime = 0.0f;
			
			// Handling reverse playback
			if (reverse) {
				animationTime = fmod(duration - time, duration);
			} else {
				animationTime = fmod(time, duration);
			}
			
			// Evaluating the animation at the adjusted time
			evaluate_animation(animation, animationTime);
			
			// Updating the skeleton with the new transforms
			apply_pose_to_skeleton();
		}
	}
	
private:
	PlaybackComponent& mProvider;
	SkeletonComponent& mSkeletonComponent;
	AnimationTimeProvider& mAnimationTimeProvider;
	int mRegistrationId;
	bool mFrozen;
	
private:
	// Adding a keyframe to the animation
	void addKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData) {
		// Checking if a keyframe at this time already exists
		if (keyframeExists(time)) {
			// Updating the existing keyframe
			updateKeyframe(time, state, modifier, trigger, playbackData);
			return;
		}
		auto keyframe = std::make_shared<PlaybackComponent::Keyframe>(time, state, modifier, trigger, playbackData);
		keyframes_.push_back(keyframe);
		// Keeping keyframes sorted
		std::sort(keyframes_.begin(), keyframes_.end(),
				  [](const std::shared_ptr<PlaybackComponent::Keyframe>& a, const std::shared_ptr<PlaybackComponent::Keyframe>& b) {
			return a->time < b->time;
		});
		
		updateAnimationOffset(time);
	}
	
	// Updating an existing keyframe at a specified time
	void updateKeyframe(float time, PlaybackState state, PlaybackModifier modifier, PlaybackTrigger trigger, std::shared_ptr<PlaybackData> playbackData) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			(*it)->setPlaybackState(state);
			(*it)->setPlaybackModifier(modifier);
			(*it)->setPlaybackTrigger(trigger);
			(*it)->setPlaybackData(playbackData);
		} else {
			// If the keyframe does not exist, add it
			addKeyframe(time, state, modifier, trigger, playbackData);
		}
		
		updateAnimationOffset(time);
	}
	
	// Removing a keyframe at the specified time
	void removeKeyframe(float time) {
		auto it = findKeyframe(time);
		if (it != keyframes_.end()) {
			keyframes_.erase(it);
		}
		// Optionally handling the case where the keyframe does not exist
		
		updateAnimationOffset(time);
	}
	
	// Evaluating the playback state at a given time
	std::shared_ptr<PlaybackComponent::Keyframe> evaluate(float time) {
		if (keyframes_.empty()) {
			return nullptr; // Default state if no keyframes
		}
		
		// If time is before the first keyframe
		if (time < keyframes_.front()->time) {
			// Setting the model pose to the first keyframe's pose
			evaluate_animation(keyframes_.front()->getPlaybackData()->get_animation(), keyframes_.front()->time);
			
			return nullptr;
		}
		
		// If time is after the last keyframe
		if (time > keyframes_.back()->time) {
			// Setting the model pose to the last keyframe's pose
			evaluate_animation(keyframes_.back()->getPlaybackData()->get_animation(), keyframes_.back()->time);
			
			return keyframes_.back();
		}
		
		// Finding the first keyframe with time >= given time
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const std::shared_ptr<PlaybackComponent::Keyframe>& kf, float t) {
			return kf->time < t;
		});
		
		if (it != keyframes_.end()) {
			if ((*it)->time == time) {
				return *it;
			} else if (it != keyframes_.begin()) {
				// No exact match; use the closest keyframe to the left
				return *(it - 1);
			}
		}
		
		// Fallback (should not reach here if bounds are respected)
		return nullptr;
	}
	
	// Checking if the current time corresponds to an exact keyframe
	bool is_keyframe(float time) const {
		return keyframeExists(time);
	}
	
	// Getting the previous keyframe before the given time
	std::shared_ptr<PlaybackComponent::Keyframe> get_previous_keyframe(float time) {
		if (keyframes_.empty()) return nullptr;
		
		auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](const std::shared_ptr<PlaybackComponent::Keyframe>& kf, float t) {
			return kf->time < t;
		});
		
		if (it == keyframes_.begin()) return nullptr; // No previous keyframe
		--it;
		return *it;
	}
	
	// Getting the next keyframe after the given time
	std::shared_ptr<PlaybackComponent::Keyframe> get_next_keyframe(float time) {
		if (keyframes_.empty()) return nullptr;
		
		auto it = std::upper_bound(keyframes_.begin(), keyframes_.end(), time,
								   [](float t, const std::shared_ptr<PlaybackComponent::Keyframe>& kf) {
			return t < kf->time;
		});
		if (it == keyframes_.end()) return nullptr; // No next keyframe
		return *it;
	}
	
	float getAdjustedAnimationTime(float currentTime, float duration) {
		float accumulatedPlayTime = 0.0f;
		
		// If there are no keyframes, the animation remains at the initial offset
		if (keyframes_.empty()) {
			return 0.0f;
		}
		
		updateAnimationOffset(currentTime);
		
		// Initializing lastState and lastStateChangeTime based on the first keyframe
		float lastStateChangeTime = mAnimationOffset;
		PlaybackState lastState = keyframes_[0]->getPlaybackState();
		
		size_t keyframeIndex = 0;
		
		// If the first keyframe is after currentTime, the animation hasn't started yet
		if (keyframes_[0]->time > currentTime) {
			return 0.0f;
		}
		
		// Processing keyframes
		for (; keyframeIndex < keyframes_.size(); ++keyframeIndex) {
			const auto& keyframe = keyframes_[keyframeIndex];
			if (keyframe->time > currentTime) {
				break;
			}
			
			// Handling state changes
			PlaybackState currentState = keyframe->getPlaybackState();
			
			if (currentState != lastState) {
				if (lastState == PlaybackState::Play) {
					// Accumulating the duration from the last play state to the current keyframe time
					accumulatedPlayTime += keyframe->time - lastStateChangeTime;
				}
				lastState = currentState;
				lastStateChangeTime = keyframe->time;
			}
		}
		
		// After processing keyframes up to currentTime
		if (lastState == PlaybackState::Play) {
			// Accumulating time from last state change to current time
			accumulatedPlayTime += currentTime - lastStateChangeTime;
		}
		
		// Wrapping the accumulated play time around the animation duration
		float adjustedTime = fmod(accumulatedPlayTime, duration);
		
		return adjustedTime;
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> evaluate_keyframe(std::shared_ptr<PlaybackComponent::Keyframe> keyframe, float time) {
		std::optional<std::reference_wrapper<Animation>> animation;
		
		if (keyframe) {
			animation = keyframe->getPlaybackData()->get_animation();
		} else {
			return nullptr;
		}
		
		float duration = static_cast<float>(animation->get().get_duration());
		
		PlaybackModifier currentModifier = PlaybackModifier::Forward;
		
		if (keyframe) {
			currentModifier = keyframe->getPlaybackModifier();
		}
		
		bool reverse = currentModifier == PlaybackModifier::Reverse;
		float animationTime = 0.0f;
		
		// Calculating adjusted animation time
		animationTime = getAdjustedAnimationTime(time, duration);
		
		// Handling reverse playback
		if (reverse) {
			animationTime = fmod(duration - animationTime, duration);
		}
		
		// Evaluating the animation at the adjusted time
		evaluate_animation(animation->get(), animationTime);
		
		// Updating the skeleton with the new transforms
		apply_pose_to_skeleton();
		
		return keyframe;
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> get_keyframe(float time) {
		auto it = findKeyframe(time);
		
		if (it != keyframes_.end()) {
			return *it;
		} else {
			return nullptr;
		}
	}
	
private:
	// Checking if a keyframe exists at the given time
	bool keyframeExists(float time) const {
		return std::any_of(keyframes_.begin(), keyframes_.end(),
						   [time](const std::shared_ptr<PlaybackComponent::Keyframe>& kf) { return kf->time == time; });
	}
	
	// Finding a keyframe at the given time
	typename std::vector<std::shared_ptr<PlaybackComponent::Keyframe>>::iterator findKeyframe(float time) {
		return std::find_if(keyframes_.begin(), keyframes_.end(),
							[time](const std::shared_ptr<PlaybackComponent::Keyframe>& kf) { return kf->time == time; });
	}
	
	typename std::vector<std::shared_ptr<PlaybackComponent::Keyframe>>::const_iterator findKeyframe(float time) const {
		return std::find_if(keyframes_.cbegin(), keyframes_.cend(),
							[time](const std::shared_ptr<PlaybackComponent::Keyframe>& kf) { return kf->time == time; });
	}
	
	// Getting the current playback modifier at a given time
	std::optional<PlaybackModifier> get_current_playback_modifier(float time) {
		if (keyframes_.empty()) {
			return std::nullopt;
		}
		
		// Finding the keyframe at or before the current time
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
		Skeleton& skeleton = static_cast<Skeleton&>(mSkeletonComponent.get_skeleton());
		skeleton.compute_offsets(mModelPose);
	}
	
	void apply_pose_to_skeleton(std::vector<glm::mat4>& modelPose) {
		Skeleton& skeleton = static_cast<Skeleton&>(mSkeletonComponent.get_skeleton());
		skeleton.compute_offsets(modelPose);
	}
	
	void updateAnimationOffset(float time) {
		// Resetting offset to 0.0f by default
		mAnimationOffset = 0.0f;
		
		// Finding the keyframe with the largest time less than or equal to the given time
		auto keyframe_it = std::find_if(keyframes_.rbegin(), keyframes_.rend(),
										[time](const std::shared_ptr<PlaybackComponent::Keyframe>& kf) { return kf->time <= time; });
		
		// Checking if the iterator is valid
		if (keyframe_it != keyframes_.rend()) {
			auto& currentAnimation = keyframes_.front()->getPlaybackData()->get_animation();
			
			// Looping backwards through the keyframes starting from the found keyframe
			for (auto it = keyframe_it; it != keyframes_.rend(); ++it) {
				if (&(*it)->getPlaybackData()->get_animation() != &currentAnimation) {
					// If the animation has changed, reset the offset and break
					mAnimationOffset = 0.0f;
					break;
				} else {
					mAnimationOffset = (*it)->time;  // Set offset to the found keyframe's time
				}
			}
		}
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> getPreviousPlayStateKeyframe(float fromTime) {
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			auto kf = *it;
			if (kf->time < fromTime && kf->getPlaybackState() == PlaybackState::Play) {
				return kf;
			}
		}
		
		return nullptr;
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> getPreviousPauseStateKeyframe(float fromTime) {
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {

			auto kf = *it;
			if (kf->time < fromTime && kf->getPlaybackState() == PlaybackState::Pause) {
				return kf;
			}
		}
		
		return nullptr;
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> getFirstButPreviousPauseStateKeyframe(float fromTime) {
		std::shared_ptr<PlaybackComponent::Keyframe> keyframe;
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			auto kf = *it;
			if (kf->time < fromTime && kf->getPlaybackState() == PlaybackState::Play) {
				return keyframe;
			} else if (kf->time < fromTime) {
				keyframe = kf;
			}
		}
		
		return nullptr;
	}
	
	std::shared_ptr<PlaybackComponent::Keyframe> getFirstButPreviousPlayStateKeyframe(float fromTime) {
		std::shared_ptr<PlaybackComponent::Keyframe> keyframe;
		for (auto it = keyframes_.rbegin(); it != keyframes_.rend(); ++it) {
			auto kf = *it;
			if (kf->time < fromTime && kf->getPlaybackState() == PlaybackState::Pause) {
				return keyframe;
			} else if (kf->time < fromTime) {
				keyframe = kf;
			}
		}
		
		return nullptr;
	}
	
	// Blending two keyframes by interpolating each bone's transformation
	void blend_keyframes(const std::shared_ptr<PlaybackComponent::Keyframe>& current,
						 const std::shared_ptr<PlaybackComponent::Keyframe>& next,
						 float time,
						 float t) {
		if (!current->getPlaybackData() || !next->getPlaybackData()) {
			std::cerr << "Invalid playback data or animation in keyframes." << std::endl;
			return;
		}
		
		// Retrieving the associated animations
		Animation& animationA = current->getPlaybackData()->get_animation();
		Animation& animationB = next->getPlaybackData()->get_animation();
		
		// Getting the total number of bones from the skeleton
		size_t numBones = mSkeletonComponent.get_skeleton().num_bones();
		
		// Initializing blended pose
		std::vector<glm::mat4> blendedPose(numBones, glm::mat4(1.0f));
		
		const std::vector<Animation::KeyFrame> boneKeyframesA = animationA.evaluate_keyframes(time);
		const std::vector<Animation::KeyFrame> boneKeyframesB = animationB.evaluate_keyframes(time);
		
		// Blending the two transformations based on factor t
		for (size_t boneIndex = 0; boneIndex < numBones; ++boneIndex) {
			auto blendedKeyframe = Animation::blendKeyframes(boneKeyframesA[boneIndex], boneKeyframesB[boneIndex], t);
			
			glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), blendedKeyframe.translation);
			glm::mat4 rotation_matrix = glm::mat4_cast(blendedKeyframe.rotation);
			glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), blendedKeyframe.scale);
			
			glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
			
			blendedPose[boneIndex] = bone_transform;
		}
		
		// Updating the internal pose buffer
		mModelPose = blendedPose;
		
		// Applying the blended pose to the skeleton
		apply_pose_to_skeleton();
	}
	
	// Interpolating a bone's pose based on its keyframes and the interpolation factor
	glm::mat4 interpolate_bone_pose(const std::vector<Animation::KeyFrame>& keyframes, float t) const {
		// Handling edge cases
		if (keyframes.empty()) {
			return glm::mat4(1.0f);
		}
		
		if (t <= 0.0f) {
			return construct_transform_matrix(keyframes.front());
		}
		
		if (t >= 1.0f) {
			return construct_transform_matrix(keyframes.back());
		}
		
		// Finding the two keyframes surrounding the interpolation factor t
		size_t nextKeyframeIdx = 0;
		for (; nextKeyframeIdx < keyframes.size(); ++nextKeyframeIdx) {
			if ((keyframes.begin() + nextKeyframeIdx)->time >= t) {
				break;
			}
		}
		
		size_t currentKeyframeIdx = (nextKeyframeIdx == 0) ? 0 : nextKeyframeIdx - 1;
		
		const Animation::KeyFrame& keyA = keyframes[currentKeyframeIdx];
		const Animation::KeyFrame& keyB = keyframes[nextKeyframeIdx];
		
		// Calculating local interpolation factor between keyA and keyB
		float segmentDuration = keyB.time - keyA.time;
		float local_t = (t - keyA.time) / segmentDuration;
		
		// Interpolating translation and scale linearly
		glm::vec3 blendedTranslation = glm::mix(keyA.translation, keyB.translation, local_t);
		glm::vec3 blendedScale = glm::mix(keyA.scale, keyB.scale, local_t);
		
		// Interpolating rotation using SLERP
		glm::quat blendedRotation = glm::slerp(keyA.rotation, keyB.rotation, local_t);
		
		// Recomposing the blended transformation matrix
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), blendedTranslation);
		glm::mat4 rotation_matrix = glm::mat4_cast(blendedRotation);
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), blendedScale);
		
		return translation_matrix * rotation_matrix * scale_matrix;
	}
	
	// Blending two transformation matrices based on the interpolation factor
	glm::mat4 blend_transformations(const glm::mat4& transformA, const glm::mat4& transformB, float t) const {
		// Decomposing matrices into translation, rotation, and scale
		glm::vec3 translationA, scaleA, skewA;
		glm::vec4 perspectiveA;
		glm::quat rotationA;
		glm::decompose(transformA, scaleA, rotationA, translationA, skewA, perspectiveA);
		
		glm::vec3 translationB, scaleB, skewB;
		glm::quat rotationB;
		glm::vec4 perspectiveB;
		glm::decompose(transformB, scaleB, rotationB, translationB, skewB, perspectiveB);
		
		// Interpolating translation and scale linearly
		glm::vec3 blendedTranslation = glm::mix(translationA, translationB, t);
		glm::vec3 blendedScale = glm::mix(scaleA, scaleB, t);
		
		// Interpolating rotation using SLERP
		glm::quat blendedRotation = glm::slerp(rotationA, rotationB, t);
		
		// Recomposing the blended transformation matrix
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), blendedTranslation);
		glm::mat4 rotation_matrix = glm::mat4_cast(blendedRotation);
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), blendedScale);
		
		return translation_matrix * rotation_matrix * scale_matrix;
	}
	
	// Constructing a transformation matrix from a KeyFrame
	glm::mat4 construct_transform_matrix(const Animation::KeyFrame& keyframe) const {
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), keyframe.translation);
		glm::mat4 rotation_matrix = glm::mat4_cast(keyframe.rotation);
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), keyframe.scale);
		return translation_matrix * rotation_matrix * scale_matrix;
	}
	
	float mAnimationOffset; // Animation offset time
	std::vector<glm::mat4> mModelPose; // Buffers to store poses
	std::vector<glm::mat4> mDefaultPose;
	std::vector<std::shared_ptr<PlaybackComponent::Keyframe>> keyframes_; // Keyframes for playback control
	PlaybackState mLastPlaybackState = PlaybackState::Pause; // Current state tracking
};
