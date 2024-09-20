#pragma once

#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/allocator.h>


#include <iostream>
#include <memory>


class SkinnedAnimationComponent {
public:
	struct SkinnedAnimationPdo {
		SkinnedAnimationPdo(ozz::animation::Skeleton& skeleton) : mSkeleton(skeleton) {}
		
		std::reference_wrapper<ozz::animation::Skeleton> mSkeleton;
		std::vector<std::reference_wrapper<ozz::animation::Animation>> mAnimationData;
	};
	
public:
	SkinnedAnimationComponent(SkinnedAnimationPdo& animationPdo)
	: mSkeleton(animationPdo.mSkeleton.get()), mCurrentTime(0.0f) // Initialize current time
	{
		for (auto& animation : animationPdo.mAnimationData) {
			mAnimationData.push_back(animation);
		}
		
		// Initialize the pose buffer
		const int num_joints = mSkeleton.get().num_joints();
		mLocalPose.resize(num_joints);
		mModelPose.resize(num_joints);
	}
	
	void apply_to(ShaderWrapper& shader) {
		compute_bind_pose();

		if (!mAnimationData.empty()) {
			// Sample the first animation for simplicity
			sample_animation(mAnimationData[0].get(), mCurrentTime);
		} 
		
		// Convert local pose to model-space pose
		compute_model_pose();
		
		// Extract bone matrices and pass them to the shader
		std::vector<ozz::math::Float4x4> boneMatrices;
		boneMatrices.reserve(mSkeleton.get().num_joints());
		
		for (int i = 0; i < mSkeleton.get().num_joints(); ++i) {
			boneMatrices.emplace_back(mModelPose[i]);
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
	// Function to sample an animation at a given time
	// Function to sample an animation at a given time
	void sample_animation(ozz::animation::Animation& animation, float time) {
		// Initialize the SamplingJob::Context
		ozz::animation::SamplingJob::Context sampling_context;
//		sampling_context.allocator = ozz::memory::default_allocator();
		
		// Set up the SamplingJob
		ozz::animation::SamplingJob sampling_job;
		sampling_job.animation = &animation;
		sampling_job.context = &sampling_context;
		sampling_job.ratio = time / animation.duration(); // Normalize time
		sampling_job.output = ozz::make_span(mLocalPose);
		
		// Run the SamplingJob
		if (!sampling_job.Run()) {
			// Handle sampling error
			std::cerr << "Failed to run SamplingJob." << std::endl;
		}
	}

	// Function to compute the model-space pose from the local pose
	void compute_model_pose() {
		ozz::animation::LocalToModelJob ltm_job;
		ltm_job.skeleton = &mSkeleton.get();
		ltm_job.input = ozz::make_span(mLocalPose);
		ltm_job.output = ozz::make_span(mModelPose);
		
		if (!ltm_job.Run()) {
			// Handle conversion error
			std::cerr << "Failed to run LocalToModelJob." << std::endl;
		}
	}
	
	// Function to use the bind pose when no animations are present
	// Function to use the bind pose when no animations are present
	void compute_bind_pose() {
		const int num_joints = mSkeleton.get().num_joints();
		for (int i = 0; i < num_joints; ++i) {
			const ozz::math::SoaTransform& joint_rest_pose = mSkeleton.get().joint_rest_poses()[i];
			mLocalPose[i].translation = joint_rest_pose.translation;
			mLocalPose[i].rotation = joint_rest_pose.rotation;
			mLocalPose[i].scale = joint_rest_pose.scale;
		}
		compute_model_pose();
	}


private:
	std::reference_wrapper<ozz::animation::Skeleton> mSkeleton;
	std::vector<std::reference_wrapper<ozz::animation::Animation>> mAnimationData;
	
	float mCurrentTime; // Current animation time
	
	// Buffers to store poses
	std::vector<ozz::math::SoaTransform> mLocalPose;
	std::vector<ozz::math::Float4x4> mModelPose;
};
