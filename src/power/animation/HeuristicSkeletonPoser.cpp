#include "HeuristicSkeletonPoser.hpp"

#include "IBone.hpp"
#include "ISkeleton.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> // For glm::translate
#include <glm/gtx/matrix_decompose.hpp> // For glm::decompose
#include <glm/gtc/constants.hpp> // For glm::pi and glm::degrees
#include <glm/gtc/epsilon.hpp>   // For epsilon checks
#include <glm/gtx/quaternion.hpp> // For glm::eulerAngles, etc.

#include <unordered_map>
#include <functional>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>

HeuristicSkeletonPoser::HeuristicSkeletonPoser(ISkeleton& skeleton) : mSkeleton(skeleton) {
}

void HeuristicSkeletonPoser::apply() {
	// Step 1: Collect all skeleton bones
	std::vector<IBone*> bones = CollectSkeletonBones(mSkeleton);
	if (bones.empty()) {
		std::cerr << "No skeleton bones found in the actor.\n";
		return;
	}
	
	// Step 2: Classify Bones based on heuristics using global transforms
	std::vector<ClassifiedBone> classifiedBones = ClassifyBones(bones);
	
	// Step 4: Apply T-Pose Rotations
	ApplyTPose(classifiedBones, mSkeleton);
}

// Implement private member functions

double HeuristicSkeletonPoser::AngleBetween(const glm::vec3& v1, const glm::vec3& v2) {
	float dot = glm::dot(glm::normalize(v1), glm::normalize(v2));
	dot = glm::clamp(dot, -1.0f, 1.0f); // Clamp to handle numerical inaccuracies
	double angle = glm::degrees(acos(dot));
	return angle;
}

std::vector<IBone*> HeuristicSkeletonPoser::CollectSkeletonBones(ISkeleton& skeleton) {
	std::vector<IBone*> bones;
	size_t numBones = skeleton.num_bones();
	for (size_t i = 0; i < numBones; ++i) {
		bones.push_back(&skeleton.get_bone(i));
	}
	return bones;
}

IBone* HeuristicSkeletonPoser::FindRootBone(const std::vector<IBone*>& bones) {
	for (auto bone : bones) {
		int index = bone->get_parent_index();
		if (index == -1) {
			return bone;
		}
	}
	return nullptr; // If not found
}

IBone* HeuristicSkeletonPoser::GetParentBone(IBone* bone, const std::vector<IBone*>& bones) {
	int parent_index = bone->get_parent_index();
	
	if (parent_index == -1) {
		return nullptr;
	} else {
		return bones[parent_index];
	}
}

glm::mat4 HeuristicSkeletonPoser::GetBoneGlobalTransform(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	glm::mat4 localTransform = bone->get_transform_matrix(); // Assuming this function exists
	
	glm::vec3 childScale, childTranslation, childSkew;
	glm::vec4 childPerspective;
	glm::quat childRotation;
	glm::decompose(localTransform, childScale, childRotation, childTranslation, childSkew, childPerspective);
	
	IBone* parentBone = parentMap.at(bone);
	if (parentBone == nullptr) {
		// Bone has no parent, so its global transform is its local transform
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), childTranslation);
		glm::mat4 rotationMatrix = glm::mat4_cast(childRotation);
		
		return translationMatrix * rotationMatrix;
	} else {
		// Recursively compute the parent's global transform
		glm::mat4 parentGlobalTransform = GetBoneGlobalTransform(parentBone, parentMap);
		
		glm::vec3 parentScale, parentTranslation, parentSkew;
		glm::vec4 parentPerspective;
		glm::quat parentRotation;
		glm::decompose(parentGlobalTransform, parentScale, parentRotation, parentTranslation, parentSkew, parentPerspective);
		
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), childTranslation + parentTranslation);
		glm::mat4 rotationMatrix = glm::mat4_cast(childRotation * parentRotation);
		
		return translationMatrix * rotationMatrix;
	}
}

std::tuple<glm::vec3, glm::quat, glm::vec3> HeuristicSkeletonPoser::DecomposeTransform(const glm::mat4& transform) {
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return { translation, rotation, scale };
}

glm::vec3 HeuristicSkeletonPoser::GetGlobalTranslation(IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	// Get global transformation matrices
	glm::mat4 childGlobalTransform = GetBoneGlobalTransform(childBone, parentMap);
	
	glm::vec3 childScale, childTranslation, childSkew;
	glm::vec4 childPerspective;
	glm::quat childRotation;
	glm::decompose(childGlobalTransform, childScale, childRotation, childTranslation, childSkew, childPerspective);
	
	return childTranslation;
}

float HeuristicSkeletonPoser::AngleBetween(const glm::vec4& v1, const glm::vec4& v2) {
	// Calculate the dot product of the two vectors
	float dotProduct = glm::dot(v1, v2);
	
	// Calculate the magnitudes (lengths) of the vectors
	float magnitude1 = glm::length(v1);
	float magnitude2 = glm::length(v2);
	
	// Check for zero-length vectors to avoid division by zero
	if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
		return 0.0f;
	}
	
	// Calculate the angle in radians between the vectors
	float angleRadians = glm::acos(dotProduct / (magnitude1 * magnitude2));
	
	// Convert the angle from radians to degrees
	float angleDegrees = glm::degrees(angleRadians);
	
	return angleDegrees;
}

bool HeuristicSkeletonPoser::IsBoneBelow(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	glm::vec3 parentTranslation = GetGlobalTranslation(parentBone, parentMap);
	glm::vec3 childTranslation = GetGlobalTranslation(childBone, parentMap);
	return parentTranslation.z > childTranslation.z; // Adjust the threshold as needed
}

bool HeuristicSkeletonPoser::IsBoneAbove(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	glm::vec3 parentTranslation = GetGlobalTranslation(parentBone, parentMap);
	glm::vec3 childTranslation = GetGlobalTranslation(childBone, parentMap);
	return parentTranslation.z < childTranslation.z; // Adjust the threshold as needed
}

bool HeuristicSkeletonPoser::IsBoneOnLeft(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	glm::vec3 parentTranslation = GetGlobalTranslation(parentBone, parentMap);
	glm::vec3 childTranslation = GetGlobalTranslation(childBone, parentMap);
	return parentTranslation.x > childTranslation.x; // Adjust the threshold as needed
}

bool HeuristicSkeletonPoser::IsBoneOnRight(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	glm::vec3 parentTranslation = GetGlobalTranslation(parentBone, parentMap);
	glm::vec3 childTranslation = GetGlobalTranslation(childBone, parentMap);
	return parentTranslation.x < childTranslation.x; // Adjust the threshold as needed
}

std::vector<HeuristicSkeletonPoser::ClassifiedBone> HeuristicSkeletonPoser::ClassifyBones(const std::vector<IBone*>& bones) {
	std::vector<ClassifiedBone> classifiedBones;
	std::unordered_map<IBone*, BoneType> boneTypeMap;
	std::unordered_map<IBone*, IBone*> parentMap;
	
	// Build the parent map
	for (auto bone : bones) {
		IBone* parent = GetParentBone(bone, bones);
		parentMap[bone] = parent; // parent can be nullptr if bone is the root
	}
	
	// Find the root bone
	IBone* rootBone = FindRootBone(bones);
	if (!rootBone) {
		std::cerr << "Root bone not found.\n";
		return classifiedBones;
	}
	
	// Assign root bone
	classifiedBones.emplace_back(ClassifiedBone{ rootBone, BoneType::Root });
	boneTypeMap[rootBone] = BoneType::Root;
	
	// Recursive traversal function to classify bones based on bone chains using heuristics
	std::function<void(IBone*, BoneType)> TraverseAndClassify = [&](IBone* bone, BoneType parentType) {
		// Get the children of the bone
		std::vector<IBone*> children;
		for (auto childBone : bones) {
			if (GetParentBone(childBone, bones) == bone) {
				children.push_back(childBone);
			}
		}
		
		for (auto child : children) {
			// Determine the type of the child bone based on heuristics
			BoneType childType = BoneType::Unknown;
			
			// Use relative positions
			if (parentType == BoneType::Root || parentType == BoneType::Unknown || parentType == BoneType::RootChain) {
				// Before hips are found, consider bones as part of the root chain
				// hierarchy starts here
				if (IsHipsBone(child, parentMap)) {
					childType = BoneType::Hips;
				} else {
					childType = BoneType::RootChain;
				}
			}
			else if (parentType == BoneType::Hips) {
				// After hips, check for pelvis or proceed to classify legs and spine
				if (IsPelvisBone(child, parentMap) && parentType == BoneType::Hips) {
					childType = BoneType::Pelvis;
				} else {
					childType = BoneType::Spine;
				}
			}
			else if (parentType == BoneType::Pelvis) {
				// Children of hips could be legs or spine
				if (IsLikelyLeg(child, parentMap)) {
					// Bone is below hips, likely a leg
					if (IsBoneOnLeft(bone, child, parentMap)) {
						childType = BoneType::LeftUpperLeg;
					} else {
						childType = BoneType::RightUpperLeg;
					}
				} else {
					childType = BoneType::Unclassified;
				}
			}
			else if (parentType == BoneType::Spine) {
				
				// Children of hips could be legs or spine
				if (IsLikelyArm(child, parentMap)) {
					// Bone is below hips, likely a leg
					if (IsBoneOnLeft(bone, child, parentMap)) {
						childType = BoneType::LeftUpperArm;
					} else {
						childType = BoneType::RightUpperArm;
					}
				} else {
					if (child->get_child_indices().size() == 0) {
						childType = BoneType::Head;
					} else {
						childType = BoneType::Spine;
					}
				}
			}
			else if (parentType == BoneType::LeftUpperLeg) {
				childType = BoneType::LeftLowerLeg;
			}
			else if (parentType == BoneType::LeftLowerLeg) {
				childType = BoneType::LeftFoot;
			}
			else if (parentType == BoneType::RightUpperLeg) {
				childType = BoneType::RightLowerLeg;
			}
			else if (parentType == BoneType::RightLowerLeg) {
				childType = BoneType::RightFoot;
			}
			else if (parentType == BoneType::LeftUpperArm) {
				childType = BoneType::LeftLowerArm;
			}
			else if (parentType == BoneType::LeftLowerArm) {
				childType = BoneType::LeftHand;
			}
			else if (parentType == BoneType::RightUpperArm) {
				childType = BoneType::RightLowerArm;
			}
			else if (parentType == BoneType::RightLowerArm) {
				childType = BoneType::RightHand;
			}
			else {
				// Inherit from parent if uncertain
				childType = parentType;
			}
			
			// Add to classified bones
			classifiedBones.emplace_back(ClassifiedBone{ child, childType });
			boneTypeMap[child] = childType;
			
			// Recursively classify children
			TraverseAndClassify(child, childType);
		}
	};
	
	// Start traversal from the root bone
	TraverseAndClassify(rootBone, BoneType::Root);
	
	// Optional: Print classified bones for debugging
	for (const auto& cb : classifiedBones) {
		std::cout << "Bone: " << cb.bone->get_name() << " - Type: " << BoneTypeToString(cb.type) << "\n";
	}
	
	return classifiedBones;
}

// Helper function to determine if a bone is the hips bone
bool HeuristicSkeletonPoser::IsPelvisBone(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	
	if (bone->get_child_indices().size() == 2) {
		
		glm::vec4 forward(0, 0, -1, 1);  // Z-axis
		
		glm::vec3 bonePos = GetGlobalTranslation(bone, parentMap);
		
		glm::mat4 childGlobalTransform = GetBoneGlobalTransform(bone, parentMap);
		glm::vec4 boneY = childGlobalTransform[1]; // Y-axis direction
		double angleUp = AngleBetween(boneY, forward);
		
		if (angleUp > 90 && angleUp < 135) {
			return true;
		} else {
			return false;
		}
		
	} else {
		return false;
	}
}

bool HeuristicSkeletonPoser::IsHipsBone(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	
	if (bone->get_child_indices().size() == 2) {
		
		glm::vec4 forward(0, 0, -1, 1);  // Z-axis
		
		glm::vec3 bonePos = GetGlobalTranslation(bone, parentMap);
		
		glm::mat4 childGlobalTransform = GetBoneGlobalTransform(bone, parentMap);
		glm::vec4 boneY = childGlobalTransform[1]; // Y-axis direction
		double angleUp = AngleBetween(boneY, forward);
		
		if (angleUp > 30.0 && angleUp < 90.0) {
			return true;
		} else {
			return false;
		}
		
	} else {
		return false;
	}
}

// Helper function to determine if a bone is likely a leg
bool HeuristicSkeletonPoser::IsLikelyLeg(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	
	glm::vec4 forward(0, 0, -1, 1);  // Z-axis
	
	glm::vec3 bonePos = GetGlobalTranslation(bone, parentMap);
	
	glm::mat4 childGlobalTransform = GetBoneGlobalTransform(bone, parentMap);
	glm::vec4 boneY = childGlobalTransform[1]; // Y-axis direction
	double angleUp = AngleBetween(boneY, forward);
	
	// Determine orientation alignment
	double verticalAlignment = angleUp; // Lower angle means more vertical
	double horizontalThreshold = 45.0; // Degrees
	
	if (verticalAlignment > horizontalThreshold) {
		return true;
	} else {
		return false;
	}
}

bool HeuristicSkeletonPoser::IsLikelyArm(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap) {
	
	glm::vec4 forward(0, 0, -1, 1);  // Z-axis
	
	glm::vec3 bonePos = GetGlobalTranslation(bone, parentMap);
	
	glm::mat4 childGlobalTransform = GetBoneGlobalTransform(bone, parentMap);
	glm::vec4 boneY = childGlobalTransform[1]; // Y-axis direction
	double angleUp = AngleBetween(boneY, forward);
	
	// Determine orientation alignment
	double verticalAlignment = angleUp; // Lower angle means more vertical
	double horizontalThreshold = 46.0; // Degrees
	
	if (verticalAlignment < horizontalThreshold) {
		return true;
	} else {
		return false;
	}
}

// Helper function to convert BoneType enum to string
std::string HeuristicSkeletonPoser::BoneTypeToString(BoneType type) const {
	switch (type) {
		case BoneType::Root: return "Root";
		case BoneType::RootChain: return "RootChain";
		case BoneType::Pelvis: return "Pelvis";
		case BoneType::Hips: return "Hips";
		case BoneType::Spine: return "Spine";
		case BoneType::Head: return "Head";
		case BoneType::LeftUpperArm: return "LeftUpperArm";
		case BoneType::LeftLowerArm: return "LeftLowerArm";
		case BoneType::LeftHand: return "LeftHand";
		case BoneType::RightUpperArm: return "RightUpperArm";
		case BoneType::RightLowerArm: return "RightLowerArm";
		case BoneType::RightHand: return "RightHand";
		case BoneType::LeftUpperLeg: return "LeftUpperLeg";
		case BoneType::LeftLowerLeg: return "LeftLowerLeg";
		case BoneType::LeftFoot: return "LeftFoot";
		case BoneType::RightUpperLeg: return "RightUpperLeg";
		case BoneType::RightLowerLeg: return "RightLowerLeg";
		case BoneType::RightFoot: return "RightFoot";
		case BoneType::Unclassified: return "Unclassified";
		default: return "Unknown";
	}
}

// Function to apply T-pose rotations based on classified bones
void HeuristicSkeletonPoser::ApplyTPose(const std::vector<ClassifiedBone>& classifiedBones, ISkeleton& skeleton) {
	for (const auto& cb : classifiedBones) {
		IBone* bone = cb.bone;
		
		// Skip bones that are unknown
		if (cb.type == BoneType::Unknown) {
			std::cout << "Skipping unknown bone: " << bone->get_name() << "\n";
			continue;
		}
		
		// Define desired rotations for T-pose
		glm::quat desiredRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Default: No rotation
		
		switch (cb.type) {
			case BoneType::Root:
				desiredRotation = glm::quat(glm::radians(glm::vec3(-180.0f, -90.0f, 0.0f)));
				break;
			case BoneType::RootChain:
				continue;
//				desiredRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)); // reset root bone orientation
				break;
				
			case BoneType::Spine:
			case BoneType::Head:
				continue;
				// Keep these bones upright; minimal rotation
//				desiredRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
				break;
				
			case BoneType::Hips:
				continue;
//				desiredRotation = glm::quat(glm::radians(glm::vec3(90.0f, 0.0f, 180.0f)));
				break;
				
			case BoneType::Pelvis:
				continue;
//				desiredRotation = glm::quat(glm::radians(glm::vec3(180.0f, 0.0f, 0.0f)));
				break;
			case BoneType::LeftUpperArm: {
				continue;
				// Rotate left upper arm outward (-90 degrees around Z-axis)
//				glm::quat previousRotation = bone->get_rotation();
//				
//				// Extract the existing rotation around the Y-axis
//				glm::vec3 eulerAngles = glm::eulerAngles(previousRotation);
//				
//				eulerAngles.z = glm::radians(-90.0f);
//				
//				desiredRotation = glm::quat(eulerAngles);
				
			} break;
				
			case BoneType::RightUpperArm: {
				continue;
				// Rotate right upper arm outward (90 degrees around Z-axis)
//				glm::quat previousRotation = bone->get_rotation();
//				
//				// Extract the existing rotation around the Y-axis
//				glm::vec3 eulerAngles = glm::eulerAngles(previousRotation);
//				
//				eulerAngles.z = glm::radians(90.0f);
//				
//				desiredRotation = glm::quat(eulerAngles);
			}
			break;
				
			case BoneType::LeftLowerArm:
			case BoneType::RightLowerArm:
			case BoneType::LeftHand:
			case BoneType::RightHand:
				continue;
				// Keep lower arms and hands straight
//				desiredRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
				break;
				
			case BoneType::LeftUpperLeg:
			case BoneType::RightUpperLeg:
				// legs must be left as is
				continue;

				// Legs should be slightly outward or straight; minimal rotation
//				desiredRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
				break;
				
			case BoneType::LeftLowerLeg:
			case BoneType::RightLowerLeg:
				// legs must be left as is
				continue;

				// Keep lower legs and feet straight
//				desiredRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
				break;
				
			case BoneType::LeftFoot:
			case BoneType::RightFoot:
				// feet must be left as is
				continue;
			default:
				// Default case, no rotation
				continue;
				break;
		}
		
		// Apply the desired rotation to the bone's local rotation
		bone->set_rotation(desiredRotation);
		
		// Optional: Log the applied rotation for debugging
		std::cout << "Applied rotation to bone: " << bone->get_name()
		<< " - Rotation: ("
		<< desiredRotation.x << ", "
		<< desiredRotation.y << ", "
		<< desiredRotation.z << ", "
		<< desiredRotation.w << ")\n";
	}
}
