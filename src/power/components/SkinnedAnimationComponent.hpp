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
		// Extract bone matrices and pass them to the shader
		std::vector<glm::mat4> boneMatrices;
		boneMatrices.reserve(mSkeleton.get().num_bones());
		
		mSkeleton.get().compute_global_transforms();
		
		for (int i = 0; i < mSkeleton.get().num_bones(); ++i) {
			boneMatrices.emplace_back(mSkeleton.get().get_bone(i).inverse_bind_pose);
		}
		
		// Set the "bones" buffer in the shader
		shader.set_buffer("bones", nanogui::VariableType::Float32, { boneMatrices.size(), 4, 4 }, boneMatrices.data());

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
