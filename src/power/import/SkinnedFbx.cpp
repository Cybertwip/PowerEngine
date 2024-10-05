#include "import/SkinnedFbx.hpp"

#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace SkinnedFbxUtil {


// return translate, rotate, scale
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4 &transform)
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, rotation, scale};
}
glm::vec3 ExtractScale(const glm::mat4& matrix) {
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));
	return scale;
}


inline glm::mat4 SfbxMatToGlmMat(const sfbx::double4x4 &from)
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

void SkinnedFbx::ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	// Get the geometry from the mesh and its deformers
	auto geometry = mesh->getGeometry();
	if (!geometry) {
		std::cerr << "Error: Mesh has no geometry." << std::endl;
		return;
	}
	
	auto deformers = geometry->getDeformers();
	const auto& vertexIndices = geometry->getIndices();
	
	std::shared_ptr<sfbx::Skin> skin;
	
	// Find the skin deformer in the mesh
	for (auto& deformer : deformers) {
		if (skin = sfbx::as<sfbx::Skin>(deformer); skin) {
			break;
		}
	}
	
	// If a skin deformer is found, proceed with processing bones
	if (skin) {
		// Retrieve the last processed mesh data
		if (GetMeshData().empty()) {
			std::cerr << "Error: No mesh data available to process bones." << std::endl;
			return;
		}
		auto& lastProcessedMesh = GetMeshData().back();
		
		// Create a new SkinnedMeshData instance and add it to mSkinnedMeshes
		mSkinnedMeshes.push_back(std::make_unique<SkinnedMeshData>(*lastProcessedMesh));
		
		// Initialize skinned vertices
		auto& skinnedVertices = mSkinnedMeshes.back()->get_vertices();
		
		// Get all the skin clusters (bones) attached to the mesh
		auto skinClusters = skin->getClusters();
		if (skinClusters.empty()) {
			std::cerr << "Warning: No skin clusters found for the mesh." << std::endl;
		}
		
		// Constants
		const int MAX_BONES = 128;
		const int MAX_BONE_INFLUENCE = 4;
		const int DEFAULT_BONE_ID = 0; // Ensure this bone exists in your shader and skeleton
		
		// Process skin clusters
		for (const auto& clusterObj : skinClusters) {
			auto skinCluster = sfbx::as<sfbx::Cluster>(clusterObj);
			if (!skinCluster) {
				std::cerr << "Error: Invalid skin cluster object." << std::endl;
				continue;
			}
			
			// Use getLink() to retrieve the bone associated with the cluster
			auto model = sfbx::as<sfbx::Model>(skinCluster->getChild());
			if (!model) {
				std::cerr << "Error: Skin cluster's link is not a valid model." << std::endl;
				continue;
			}
			
			std::string boneName = std::string{ model->getName() };
			if (boneName.empty()) {
				std::cerr << "Error: Bone name is empty." << std::endl;
				continue;
			}
			
			// Assign bone ID if not already mapped
			if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
				if (mBoneMapping.size() >= MAX_BONES) {
					std::cerr << "Error: Exceeded maximum number of bones (" << MAX_BONES << "). Bone " << boneName << " cannot be assigned." << std::endl;
					continue; // Skip this bone
				}
				
				int newBoneID = static_cast<int>(mBoneMapping.size());
				mBoneMapping[boneName] = newBoneID;
				
				// Get the inverse bind pose matrix from the skin cluster
				glm::mat4 offsetMatrix = glm::inverse(SkinnedFbxUtil::SfbxMatToGlmMat(skinCluster->getTransformLink()));
				
				// Get the bone's bind pose (global transform at bind time)
				glm::mat4 bindPoseMatrix = SkinnedFbxUtil::SfbxMatToGlmMat(model->getGlobalMatrix());
				
				// Store bone info
				BoneHierarchyInfo boneInfo;
				boneInfo.bone_id = newBoneID;
				boneInfo.offset = offsetMatrix;
				boneInfo.bindpose = bindPoseMatrix;
				boneInfo.limb = model; // Store the model
				
				// Add to hierarchy
				mBoneHierarchy.push_back(boneInfo);
				
				// Debug: Print bone info
				std::cout << "Assigned Bone: " << boneName << " with ID: " << newBoneID << std::endl;
			}
			
			// Assign weights to vertices
			int boneID = mBoneMapping[boneName];
			auto weights = skinCluster->getWeights();
			auto indices = skinCluster->getIndices();
			
			// Ensure weight and index sizes match
			if (weights.size() != indices.size()) {
				std::cerr << "Error: Mismatch between weights (" << weights.size() << ") and indices (" << indices.size() << ") for bone: " << boneName << std::endl;
				continue;
			}
			
			// Assign actual weights
			for (size_t i = 0; i < weights.size(); ++i) {
				int vertexID = indices[i];
				float weight = static_cast<float>(weights[i]);
				
				// Validate weight
				if (weight <= 0.0f) {
					continue;
				}
				
				// Ensure valid vertex ID
				if (vertexID >= 0 && static_cast<size_t>(vertexID) < skinnedVertices.size()) {
					auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[vertexID]);
					vertex.add_bone_weight(boneID, weight);
				} else {
					std::cerr << "Error: Invalid vertex ID (" << vertexID << ") for bone: " << boneName << ". Skinned Vertices Size: " << skinnedVertices.size() << std::endl;
				}
			}
		}
		
		// Normalize bone weights and limit influences
		for (auto& vertexPtr : skinnedVertices) {
			auto& vertex = static_cast<SkinnedMeshVertex&>(*vertexPtr);
			vertex.normalize_weights(MAX_BONE_INFLUENCE);
		}
		
		// Build skeleton
		auto skeletonPointer = std::make_unique<Skeleton>();
		auto& skeleton = *skeletonPointer;
		
		for (const auto& boneInfo : mBoneHierarchy) {
			std::string boneName = GetBoneNameById(boneInfo.bone_id);
			
			// Determine parent index
			int parentIndex = -1;
			auto parentModel = sfbx::as<sfbx::Model>(boneInfo.limb->getParent());
			if (parentModel) {
				std::string parentBoneName = std::string{ parentModel->getName() };
				int parentId = GetBoneIdByName(parentBoneName);
				if (parentId != -1) {
					parentIndex = parentId;
				}
			}
			
			// Add bone to skeleton
			skeleton.add_bone(boneName, boneInfo.offset, boneInfo.bindpose, parentIndex);
		}
		
		// Assign the skeleton if it has bones
		mSkeleton = skeleton.num_bones() > 0 ? std::move(skeletonPointer) : nullptr;
		
		std::cout << "Finished processing bones for skinned mesh." << std::endl;
	} else {
		std::cerr << "Warning: No skin deformer found for the mesh." << std::endl;
	}
}

std::string SkinnedFbx::GetBoneNameById(int boneId) const {
	for (const auto& [name, id] : mBoneMapping) {
		if (id == boneId) {
			return name;
		}
	}
	return "";
}

int SkinnedFbx::GetBoneIdByName(const std::string& boneName) const {
	for (const auto& [name, id] : mBoneMapping) {
		if (name == boneName) {
			return id;
		}
	}
	return -1;
}



void SkinnedFbx::TryImportAnimations() {
	if (mDoc && mDoc->valid()) {
		
		float fps = mDoc->global_settings.frame_rate;
		
		for (auto& stack : mDoc->getAnimationStacks()) {
			// Iterate through animation layers
			for (auto& animation_layer : stack->getAnimationLayers()) {
				auto animationPtr = std::make_unique<Animation>();
				Animation& animation = *animationPtr;  // Custom Animation class
				
				// Variables to track min and max keyframe times
				int min_time = 0;
				int max_time = 0;
				
				// Iterate through bone mapping to create keyframes for each bone
				for (const auto& bone : mBoneMapping) {
					const std::string& bone_name = bone.first;
					int bone_index = bone.second;
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
					
					// Determine the maximum number of frames to process
					std::size_t max_frames = 0;
					if (position_curve) {
						max_frames = std::max(max_frames, position_curve->getAnimationCurves()[0]->getTimes().size());
					}
					if (rotation_curve) {
						max_frames = std::max(max_frames, rotation_curve->getAnimationCurves()[0]->getTimes().size());
					}
					if (scale_curve) {
						max_frames = std::max(max_frames, scale_curve->getAnimationCurves()[0]->getTimes().size());
					}
					
					max_time = max_frames;
					
					// Get the inverse bind pose for this bone from the hierarchy
					auto& bone_info = mBoneHierarchy[bone_index];
					
					for (std::size_t frame_idx = 0; frame_idx < max_frames; ++frame_idx) {
						// Retrieve and evaluate the translation
						if (position_curve) {
							position_curve->applyAnimation(frame_idx / fps);
						}
						
						// Retrieve and evaluate the rotation
						if (rotation_curve) {
							rotation_curve->applyAnimation(frame_idx / fps);
						}
						
						// Retrieve and evaluate the scale
						if (scale_curve) {
							scale_curve->applyAnimation(frame_idx / fps);
						}
						
						auto model = sfbx::as<sfbx::Model>(rotation_curve->getAnimationTarget());
						
						auto [tt, rr, ss] = SkinnedFbxUtil::DecomposeTransform(SkinnedFbxUtil::SfbxMatToGlmMat(model->getLocalMatrix()));
						
						glm::mat4 transformation = glm::translate(glm::mat4(1.0f), tt) * glm::toMat4(glm::normalize(rr)) * glm::scale(glm::mat4(1.0f), ss);
						
						transformation = glm::inverse(bone_info.bindpose) * transformation;
						
						// Decompose the final transformation matrix back into position, rotation, and scale
						glm::vec3 final_position;
						glm::quat final_rotation;
						glm::vec3 final_scale;
						std::tie(final_position, final_rotation, final_scale) = SkinnedFbxUtil::DecomposeTransform(transformation);
						
						// Store the decomposed values in the keyframe
						
						Animation::KeyFrame keyframe;
						keyframe.time = frame_idx;
						keyframe.translation = final_position;
						keyframe.rotation = final_rotation;
						keyframe.scale = final_scale;
						
						keyframes.push_back(keyframe);
					}
					
					// Add the bone's keyframes to the animation
					if (!keyframes.empty()) {
						animation.add_bone_keyframes(bone_info.bone_id, keyframes);
					}
				}
				
				// Set the animation's duration based on keyframe times
				if (min_time < max_time) {
					animation.set_duration(max_time - min_time);
				} else {
					animation.set_duration(0);  // Handle case with single keyframe or no keyframes
				}
				
				// Add the animation to the mAnimations vector
				animationPtr->sort();
				mAnimations.push_back(std::move(animationPtr));
			}
		}
	}
}
