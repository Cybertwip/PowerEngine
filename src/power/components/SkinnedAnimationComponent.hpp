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

class SkinnedAnimationComponent {
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

	struct SkinnedAnimationPdo {
		SkinnedAnimationPdo(std::unique_ptr<Skeleton> skeleton) : mSkeleton(std::move(skeleton)) {}
		
		std::unique_ptr<Skeleton> mSkeleton;
		std::vector<std::reference_wrapper<Animation>> mAnimationData;
	};
	
public:
	SkinnedAnimationComponent(std::unique_ptr<SkinnedAnimationPdo> animationPdo)
	: mAnimationPdo(std::move(animationPdo)), mSkeleton(*mAnimationPdo->mSkeleton), mCurrentTime(0), mReverse(false), mPlaying(false)
	{
		for (auto& animation : mAnimationPdo->mAnimationData) {
			mAnimationData.push_back(animation);
		}
		
		// Initialize the pose buffers
		size_t numBones = mSkeleton.get().num_bones();
		mModelPose.resize(numBones);
	}
	
	void set_pdo(std::unique_ptr<SkinnedAnimationPdo> animationPdo){
		
		// skeleton does not change but must match this animation.

		animationPdo->mSkeleton = std::move(mAnimationPdo->mSkeleton);
		
		
		
		// do skeleton matching (index and bone naming)
		
		//@TODO
		
		//
		
		mAnimationPdo = std::move(animationPdo);

		
		mAnimationData.clear();
		
		for (auto& animation : mAnimationPdo->mAnimationData) {
			mAnimationData.push_back(animation);
		}

	}
	
	
	void set_reverse(bool reverse) {
		mReverse = reverse;
	}
	
	void set_playing(bool playing) {
		mPlaying = playing;
	}
	
	
	
	std::vector<BoneCPU> get_bones() {
		if (mPlaying) {
			update(1);
		} else {
			Skeleton& skeleton = mSkeleton.get();
			skeleton.compute_offsets({});
		}
		
#if defined(NANOGUI_USE_METAL)
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
#else
		// OpenGL or other rendering API code to upload bone transforms
		// For example, using a uniform array of matrices
		size_t numBones = mSkeleton.get().num_bones();
		std::vector<glm::mat4> boneTransforms(numBones);
		for (size_t i = 0; i < numBones; ++i) {
			boneTransforms[i] = mSkeleton.get().get_bone(i).transform * boneTransforms[i] = mSkeleton.get().get_bone(i).offset;
		}
		
		// Upload the boneTransforms to the shader
		shader.set_uniform("bones", boneTransforms);
#endif
		
	}
	
	// Function to update the animation time
	void update(float deltaTime) {
		if (mAnimationData.empty()) {
			return; // No animations to process
		}
		
		// For simplicity, use the first animation in the list
		const Animation& animation = mAnimationData[0].get();
		int duration = animation.get_duration();
		
		// Update current time and wrap around if necessary
		if (mReverse) {
			mCurrentTime += deltaTime * -1;
		} else {
			mCurrentTime += deltaTime * 1;
		}
				
		mCurrentTime = fmax(0, fmod(duration + mCurrentTime, duration));

		// Evaluate the animation at the current time
		evaluate_animation(animation, mCurrentTime);
		
		// Update the skeleton with the new transforms
		apply_pose_to_skeleton();
	}
	
	auto& skeleton() const { return mSkeleton.get(); }
	
private:
	void evaluate_animation(const Animation& animation, float time) {
		mModelPose = animation.evaluate(time);
	}
	
	void apply_pose_to_skeleton() {
		Skeleton& skeleton = mSkeleton.get();
		skeleton.compute_offsets(mModelPose);
	}
	
	std::unique_ptr<SkinnedAnimationPdo> mAnimationPdo;
	
	std::reference_wrapper<Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<Animation>> mAnimationData;
	
	int mCurrentTime; // Current animation time
	bool mReverse; // Current animation time
	bool mPlaying; // Current animation time
	
	// Buffers to store poses
	std::vector<glm::mat4> mModelPose;
};
