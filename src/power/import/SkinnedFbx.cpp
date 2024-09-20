#include "import/SkinnedFbx.hpp"

#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace SkinnedFbxUtil {
inline glm::mat4 SfbxMatToGlmMat(const sfbx::float4x4 &from)
{
	glm::mat4 to;
	// the a,b,c,d in sfbx is the row ; the 1,2,3,4 is the column
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			to[j][i] = from[j][i];
		}
	}
	
	return to;
}
}  // namespace

SkinnedFbx::SkinnedFbx(const std::string& path) : Fbx(path) {
}

void SkinnedFbx::ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
	auto geometry = mesh->getGeometry();
	auto deformers = geometry->getDeformers();
	std::shared_ptr<sfbx::Skin> skin;
	
	// Find the skin deformer
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
			
			// Assign bone ID if not already assigned
			if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
				int newBoneID = static_cast<int>(mBoneMapping.size());
				mBoneMapping[boneName] = newBoneID;
				
				// Retrieve the local bind pose matrix using getLocalMatrix
				glm::mat4 localBindPose = SkinnedFbxUtil::SfbxMatToGlmMat(skinCluster->getTransform());
				
				// Store bind pose and parent information
				BoneHierarchyInfo boneInfo;
				boneInfo.boneID = newBoneID;
				boneInfo.localBindPose = localBindPose;
				
				// Determine parent bone
				auto parentNode = skinCluster->getChild()->getParent(); // Assuming getParent() returns the parent node
				if (parentNode && parentNode->getSubClass() == sfbx::ObjectSubClass::LimbNode) { // Adjust Type as per your FBX SDK
					std::string parentBoneName = std::string{parentNode->getName()};
					boneInfo.parentBoneName = parentBoneName;
				} else {
					boneInfo.parentBoneName = ""; // No parent (root bone)
				}
				
				mBoneHierarchy.push_back(boneInfo);
			}
			
			std::size_t boneID = mBoneMapping[boneName];
			auto weights = skinCluster->getWeights();
			auto indices = skinCluster->getIndices();
			
			for (size_t i = 0; i < weights.size(); ++i) {
				int vertexID = indices[i];
				float weight = weights[i];
				for (int j = 0; j < 4; ++j) {
					if (mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].get_weights()[j] == 0.0f) {
						mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].set_bone(static_cast<int>(boneID), weight);
						break;
					}
				}
			}
		}
	}
}

void SkinnedFbx::TryBuildSkeleton() {
	auto skeletonPointer = std::make_unique<Skeleton>();
	auto& skeleton = *skeletonPointer;
	
	// Temporary map to find bone indices by name
	std::unordered_map<std::string, int> boneNameToIndex;
	
	for (const auto& boneInfo : mBoneHierarchy) {
		std::string boneName = GetBoneNameByID(boneInfo.boneID);
		if (boneName.empty()) {
			std::cerr << "Warning: Bone name not found for bone ID " << boneInfo.boneID << std::endl;
			continue;
		}
		
		glm::vec3 translation = glm::vec3(boneInfo.localBindPose[3]); // Extract translation from matrix
		glm::quat rotation = glm::quat_cast(boneInfo.localBindPose); // Extract rotation from matrix
		glm::vec3 scale = ExtractScale(boneInfo.localBindPose); // Extract scale
		
		// Determine parent index
		int parentIndex = -1;
		if (!boneInfo.parentBoneName.empty()) {
			auto it = boneNameToIndex.find(boneInfo.parentBoneName);
			if (it != boneNameToIndex.end()) {
				parentIndex = it->second;
			} else {
				std::cerr << "Parent bone '" << boneInfo.parentBoneName << "' not found for bone '" << boneName << "'. Setting parent_index to -1.\n";
			}
		}
		
		// Add bone to the skeleton
		skeleton.add_bone(boneName, translation, rotation, scale, parentIndex);
		
		// Update the temporary map
		boneNameToIndex[boneName] = skeleton.num_bones() - 1;
	}
	
	// Compute global transforms and inverse bind poses
	skeleton.compute_global_transforms();
	skeleton.compute_inverse_bind_poses();
	
	mSkeleton = std::move(skeletonPointer);
}

std::string SkinnedFbx::GetBoneNameByID(int boneID) const {
	for (const auto& [name, id] : mBoneMapping) {
		if (id == boneID) {
			return name;
		}
	}
	return "";
}

glm::vec3 SkinnedFbx::ExtractScale(const glm::mat4& matrix) const {
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));
	return scale;
}



void SkinnedFbx::TryImportAnimations() {
	for (auto& stack : mDoc->getAnimationStacks()) {
		// Iterate through animation layers
		for (auto& animation_layer : stack->getAnimationLayers()) {
			auto animationPtr = std::make_unique<Animation>();
			Animation& animation = *animationPtr;  // Custom Animation class
			
			// Variables to track min and max keyframe times
			float min_time = std::numeric_limits<float>::max();
			float max_time = std::numeric_limits<float>::min();
			
			// Iterate through bone mapping to create keyframes for each bone
			for (const auto& bone : mBoneMapping) {
				const std::string& bone_name = bone.first;
				std::vector<Animation::KeyFrame> keyframes;  // Store keyframes for each bone
				
				// Retrieve all animation curve nodes
				auto curve_nodes = animation_layer->getAnimationCurveNodes();
				
				// Filter for position, rotation, and scale curves
				std::shared_ptr<sfbx::AnimationCurveNode> position_curve = nullptr;
				std::shared_ptr<sfbx::AnimationCurveNode> rotation_curve = nullptr;
				std::shared_ptr<sfbx::AnimationCurveNode> scale_curve = nullptr;
				
				// Iterate over all curve nodes and assign them based on AnimationKind
				for (const auto& curve_node : curve_nodes) {
					if (curve_node->getAnimationTarget()->getName() == bone_name) {
						switch (curve_node->getAnimationKind()) {
							case sfbx::AnimationKind::Position:
								position_curve = curve_node;
								break;
							case sfbx::AnimationKind::Rotation:
								rotation_curve = curve_node;
								break;
							case sfbx::AnimationKind::Scale:
								scale_curve = curve_node;
								break;
							default:
								break;
						}
					}
				}
				
				// Collect all unique keyframe times from position, rotation, and scale curves
				std::vector<float> all_times;
				if (position_curve) {
					auto pos_times = position_curve->getAnimationCurves()[0]->getTimes();
					all_times.insert(all_times.end(), pos_times.begin(), pos_times.end());
				}
				if (rotation_curve) {
					auto rot_times = rotation_curve->getAnimationCurves()[0]->getTimes();
					all_times.insert(all_times.end(), rot_times.begin(), rot_times.end());
				}
				if (scale_curve) {
					auto scale_times = scale_curve->getAnimationCurves()[0]->getTimes();
					all_times.insert(all_times.end(), scale_times.begin(), scale_times.end());
				}
				
				// Remove duplicate times and sort
				std::sort(all_times.begin(), all_times.end());
				all_times.erase(std::unique(all_times.begin(), all_times.end()), all_times.end());
				
				// Populate keyframes with translation, rotation, and scale for the current bone
				for (const auto& key_time : all_times) {
					Animation::KeyFrame keyframe;
					keyframe.time = static_cast<float>(key_time);
					
					// Retrieve and evaluate the translation
					if (position_curve) {
						auto translation = position_curve->evaluateF3(key_time);
						keyframe.translation = glm::vec3(translation.x, translation.y, translation.z);
					} else {
						keyframe.translation = glm::vec3(0.0f);  // Default to zero if no translation
					}
					
					// Retrieve and evaluate the rotation
					if (rotation_curve) {
						auto rotation = rotation_curve->evaluateF3(key_time);
						glm::vec3 euler_angles(rotation.x, rotation.y, rotation.z);
						keyframe.rotation = glm::quat(glm::radians(euler_angles));  // Convert to quaternion
					} else {
						keyframe.rotation = glm::quat();  // Default to identity quaternion
					}
					
					// Retrieve and evaluate the scale
					if (scale_curve) {
						auto scale = scale_curve->evaluateF3(key_time);
						keyframe.scale = glm::vec3(scale.x, scale.y, scale.z);
					} else {
						keyframe.scale = glm::vec3(1.0f);  // Default to 1.0 if no scaling
					}
					
					keyframes.push_back(keyframe);
					
					// Update min and max keyframe times
					min_time = std::min(min_time, keyframe.time);
					max_time = std::max(max_time, keyframe.time);
				}
				
				// Add the bone's keyframes to the animation
				if (!keyframes.empty()) {
					animation.add_bone_keyframes(bone_name, keyframes);
				}
			}
			
			// Set the animation's duration based on keyframe times
			if (min_time < max_time) {
				animation.set_duration(max_time - min_time);
			} else {
				animation.set_duration(0.0f);  // Handle case with single keyframe or no keyframes
			}
			
			// Add the animation to the mAnimations vector
			mAnimations.push_back(std::move(animationPtr));
		}
	}
}
