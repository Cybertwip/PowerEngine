#pragma once

#include "animation/Animation.hpp"
#include "animation/Transform.hpp"
#include "animation/Skeleton.hpp"

#include <iostream>
#include <memory>


class SkinnedAnimationComponent {
public:
	struct SkinnedAnimationPdo {
		SkinnedAnimationPdo(Skeleton& skeleton) : mSkeleton(skeleton) {}
		
		std::reference_wrapper<Skeleton> mSkeleton;
		std::vector<std::reference_wrapper<Animation>> mAnimationData;
	};
	
public:
	SkinnedAnimationComponent(SkinnedAnimationPdo& animationPdo)
	: mSkeleton(animationPdo.mSkeleton.get()), mCurrentTime(0.0f) // Initialize current time
	{
		for (auto& animation : animationPdo.mAnimationData) {
			mAnimationData.push_back(animation);
		}
		
	}
	
	void apply_to(ShaderWrapper& shader) {
		
	}
	
	// Function to update the animation time (to be called externally)
	void update(float deltaTime) {
		mCurrentTime += deltaTime;
		// Handle looping or animation duration as needed
	}
	
private:
	std::reference_wrapper<Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<Animation>> mAnimationData;
	
	float mCurrentTime; // Current animation time
	
	// Buffers to store poses
	std::vector<Transform> mLocalPose;
	std::vector<glm::mat4> mModelPose;
};
