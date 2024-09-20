#pragma once

#include "animation/Animation.hpp"
#include "animation/Transform.hpp"
#include "animation/Skeleton.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>

struct BoneCPU
{
	float transform[4][4] =
	{
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f
	};
};

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
				
#if defined(NANOGUI_USE_METAL)
		// Ensure we have a valid number of materials
		size_t numBones = mSkeleton.get().num_bones();
		
		std::vector<BoneCPU> bonesCPU(numBones);
		
		for (int i = 0; i < numBones; ++i) {
			auto boneTransform = mSkeleton.get().get_bone(i).local_transform.to_matrix();
			
			BoneCPU& boneCPU = bonesCPU[i];
			
			std::memcpy(boneCPU.transform, glm::value_ptr(boneTransform), sizeof(float) * 16);
		}
		
		shader.set_buffer(
						  "bones",
						  nanogui::VariableType::Float32,
						  {numBones, sizeof(BoneCPU) / sizeof(float)},
						  bonesCPU.data()
						  );
#else
		
#endif

		
	}
	
	// Function to update the animation time (to be called externally)
	void update(float deltaTime) {
		mCurrentTime += deltaTime;
		// Handle looping or animation duration as needed
	}
	
	auto& skeleton() const { return mSkeleton.get(); }
	
private:
	std::reference_wrapper<Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<Animation>> mAnimationData;
	
	float mCurrentTime; // Current animation time
	
	// Buffers to store poses
	std::vector<Transform> mLocalPose;
	std::vector<glm::mat4> mModelPose;

};
