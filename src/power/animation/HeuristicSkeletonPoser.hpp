#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <tuple>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

class ISkeleton;
class IBone;

class HeuristicSkeletonPoser {
private:
	// Enumeration for Bone Types
	enum class BoneType {
		Unknown,
		Unclassified,
		Root,
		RootChain,
		Pelvis,
		Hips,
		Spine,
		Head,
		LeftUpperArm,
		LeftLowerArm,
		LeftHand,
		RightUpperArm,
		RightLowerArm,
		RightHand,
		LeftUpperLeg,
		LeftLowerLeg,
		LeftFoot,
		RightUpperLeg,
		RightLowerLeg,
		RightFoot
	};
	
	// Structure to hold classified bone information
	struct ClassifiedBone {
		IBone* bone;          // Pointer to the bone
		BoneType type;        // Classified type of the bone
	};
	
public:
	HeuristicSkeletonPoser(ISkeleton& skeleton);
	void apply();
	
private:
	// Private member functions
	double AngleBetween(const glm::vec3& v1, const glm::vec3& v2);
	float AngleBetween(const glm::vec4& v1, const glm::vec4& v2);
	
	std::vector<IBone*> CollectSkeletonBones(ISkeleton& skeleton);
	IBone* FindRootBone(const std::vector<IBone*>& bones);
	
	glm::mat4 GetBoneGlobalTransform(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap);
	std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform);
	
	glm::vec3 GetGlobalTranslation(IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap);
	
	bool IsBoneBelow(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsBoneAbove(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsBoneOnLeft(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsBoneOnRight(IBone* parentBone, IBone* childBone, const std::unordered_map<IBone*, IBone*>& parentMap);
	
	std::vector<ClassifiedBone> ClassifyBones(const std::vector<IBone*>& bones);
	bool IsPelvisBone(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsHipsBone(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsLikelyLeg(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap);
	bool IsLikelyArm(IBone* bone, const std::unordered_map<IBone*, IBone*>& parentMap);
	
	std::string BoneTypeToString(BoneType type) const;
	void ApplyTPose(const std::vector<ClassifiedBone>& classifiedBones, ISkeleton& skeleton);
	
	ISkeleton& mSkeleton;
};
