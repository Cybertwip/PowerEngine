#include "import/SkinnedFbx.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace SkinnedFbxUtil {
ozz::math::Transform ToOzzTransform(const sfbx::float4x4& mat) {
    // Extract translation
    ozz::math::Float3 translation(mat[3].x, mat[3].y, mat[3].z);

    // Extract scale
    ozz::math::Float3 scale(glm::length(glm::vec3(mat[0].x, mat[0].y, mat[0].z)),
                            glm::length(glm::vec3(mat[1].x, mat[1].y, mat[1].z)),
                            glm::length(glm::vec3(mat[2].x, mat[2].y, mat[2].z)));

    // Extract rotation
    glm::mat3 rotationMatrix(glm::vec3(mat[0].x, mat[0].y, mat[0].z) / scale.x,
                             glm::vec3(mat[1].x, mat[1].y, mat[1].z) / scale.y,
                             glm::vec3(mat[2].x, mat[2].y, mat[2].z) / scale.z);

    glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
    ozz::math::Quaternion rotation(rotationQuat.w, rotationQuat.x, rotationQuat.y, rotationQuat.z);

    ozz::math::Transform transform = {translation, rotation, scale};
    return transform;
}

}  // namespace

SkinnedFbx::SkinnedFbx(const std::string& path) : Fbx(path) {
}

void SkinnedFbx::ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
		
    auto geometry = mesh->getGeometry();
    auto deformers = geometry->getDeformers();
    std::shared_ptr<sfbx::Skin> skin;

    for (auto& deformer : deformers) {
        if (skin = sfbx::as<sfbx::Skin>(deformer); skin) {
            break;
        }
    }

    if (skin) {
		auto& lastProcessedMesh = GetMeshData().back();
		
		mSkinnedMeshes.push_back(std::make_unique<SkinnedMeshData>(std::move(lastProcessedMesh)));

        auto skinClusters = skin->getChildren();
        for (const auto& clusterObj : skinClusters) {
            auto skinCluster = sfbx::as<sfbx::Cluster>(clusterObj);
            std::string boneName = std::string{skinCluster->getChild()->getName()};
            if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
                mBoneMapping[boneName] = static_cast<int>(mBoneMapping.size());
				mBoneTransforms.emplace_back(SkinnedFbxUtil::ToOzzTransform(skinCluster->getTransform()));
            }

            std::size_t boneID = mBoneMapping[boneName];
            auto weights = skinCluster->getWeights();
            auto indices = skinCluster->getIndices();

            for (size_t i = 0; i < weights.size(); ++i) {
                int vertexID = indices[i];
                float weight = weights[i];
                for (int j = 0; j < 4; ++j) {
					if (mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].get_weights()[j] == 0.0f) {
						mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].set_bone(static_cast<int>(boneID),
                                                                      weight);
                        break;
                    }
                }
            }
        }
    }
}

void SkinnedFbx::TryBuildSkeleton() {
	
	// Build the Ozz skeleton
	ozz::animation::offline::RawSkeleton rawSkeleton;
	rawSkeleton.roots.resize(mBoneMapping.size());
	
	int index = 0;
	for (const auto& bone : mBoneMapping) {
		ozz::animation::offline::RawSkeleton::Joint joint;
		joint.name = bone.first.c_str();
		joint.transform = mBoneTransforms[bone.second];
		rawSkeleton.roots[index] = joint;
		index++;
	}
	
	ozz::animation::offline::SkeletonBuilder skeletonBuilder;
	auto skeleton = skeletonBuilder(rawSkeleton);
	
	mSkeleton = skeleton->num_joints() != 0 ? std::move(skeleton) : nullptr;
}

void SkinnedFbx::TryImportAnimations() {
	// Iterate through animation stacks
	for (auto& stack : mDoc->getAnimationStacks()) {
		// Iterate through animation layers
		for (auto& animationLayer : stack->getAnimationLayers()) {
			ozz::animation::offline::RawAnimation rawAnimation;
			
			// Variables to track min and max keyframe times
			float minTime = std::numeric_limits<float>::max();
			float maxTime = std::numeric_limits<float>::min();
			
			rawAnimation.tracks.resize(mBoneMapping.size());  // Resize tracks based on the number of bones
			
			// Iterate through bone mapping to create keyframes for each bone
			for (const auto& bone : mBoneMapping) {
				const std::string& boneName = bone.first;
				int boneIndex = bone.second;
				
				// Retrieve all animation curve nodes
				auto curveNodes = animationLayer->getAnimationCurveNodes();
				
				// Filter for position, rotation, and scale curves
				std::shared_ptr<sfbx::AnimationCurveNode> positionCurve = nullptr;
				std::shared_ptr<sfbx::AnimationCurveNode> rotationCurve = nullptr;
				std::shared_ptr<sfbx::AnimationCurveNode> scaleCurve = nullptr;
				
				// Iterate over all curve nodes and assign them based on AnimationKind
				for (const auto& curveNode : curveNodes) {
					if (curveNode->getAnimationTarget()->getName() == boneName) {
						switch (curveNode->getAnimationKind()) {
							case sfbx::AnimationKind::Position:
								positionCurve = curveNode;
								break;
							case sfbx::AnimationKind::Rotation:
								rotationCurve = curveNode;
								break;
							case sfbx::AnimationKind::Scale:
								scaleCurve = curveNode;
								break;
							default:
								break;
						}
					}
				}
				
				// RawAnimation::JointTrack reference for the current bone
				ozz::animation::offline::RawAnimation::JointTrack& track = rawAnimation.tracks[boneIndex];
				
				// Populate translation keyframes
				if (positionCurve) {
					for (const auto& key : positionCurve->getAnimationCurves()[0]->getTimes()) {
						ozz::animation::offline::RawAnimation::TranslationKey translationKey;
						translationKey.time = static_cast<float>(key);
						
						auto evaluation = positionCurve->evaluateF3(translationKey.time);
						translationKey.value = ozz::math::Float3(evaluation.x, evaluation.y, evaluation.z);
						
						track.translations.push_back(translationKey);
						
						// Update min and max times
						minTime = std::min(minTime, translationKey.time);
						maxTime = std::max(maxTime, translationKey.time);
					}
				}
				
				// Populate rotation keyframes
				if (rotationCurve) {
					for (const auto& key : rotationCurve->getAnimationCurves()[0]->getTimes()) {
						ozz::animation::offline::RawAnimation::RotationKey rotationKey;
						rotationKey.time = static_cast<float>(key);
						
						auto evaluation = rotationCurve->evaluateF3(rotationKey.time);
						glm::vec3 eulerAngles = glm::vec3(evaluation.x, evaluation.y, evaluation.z);
						
						// Convert the Euler angles to a quaternion
						glm::quat rotationQuat = glm::quat(glm::radians(eulerAngles)); // Assumes angles are in degrees; use radians() for safety
						
						// Set the quaternion into the ozz format
						rotationKey.value = ozz::math::Quaternion(rotationQuat.w, rotationQuat.x, rotationQuat.y, rotationQuat.z);
						
						track.rotations.push_back(rotationKey);
						
						// Update min and max times
						minTime = std::min(minTime, rotationKey.time);
						maxTime = std::max(maxTime, rotationKey.time);
					}
				}
				
				// Populate scaling keyframes
				if (scaleCurve) {
					for (const auto& key : scaleCurve->getAnimationCurves()[0]->getTimes()) {
						ozz::animation::offline::RawAnimation::ScaleKey scalingKey;
						scalingKey.time = static_cast<float>(key);
						
						auto evaluation = scaleCurve->evaluateF3(scalingKey.time);
						scalingKey.value = ozz::math::Float3(evaluation.x, evaluation.y, evaluation.z);
						
						track.scales.push_back(scalingKey);
						
						// Update min and max times
						minTime = std::min(minTime, scalingKey.time);
						maxTime = std::max(maxTime, scalingKey.time);
					}
				}
			}
			
			// Set animation duration based on keyframe times
			rawAnimation.duration = maxTime - minTime;
			
			// Build the final animation using ozz AnimationBuilder
			ozz::animation::offline::AnimationBuilder animationBuilder;
			auto animation = animationBuilder(rawAnimation);
			
			// Add the animation to the mAnimations vector
			if (animation) {
				mAnimations.push_back(std::move(animation));
			}
		}
	}
}
