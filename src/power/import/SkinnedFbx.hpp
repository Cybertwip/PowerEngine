#pragma once

#include "import/Fbx.hpp"

#include "animation/Transform.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <SmallFBX.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

class Animation;
class Fbx;
class Skeleton;

// Define BoneHierarchyInfo
struct BoneHierarchyInfo {
	int bone_id;
	glm::mat4 offset;
	glm::mat4 bindpose;
	std::shared_ptr<sfbx::LimbNode> limb;
	std::string parent_bone_name;
};

class SkinnedFbx : public Fbx {
public:
	SkinnedFbx() = default;
	~SkinnedFbx() = default;
	
	std::unique_ptr<Skeleton>& GetSkeleton() {
		return mSkeleton;
	}
	
	void SetSkeleton(std::unique_ptr<Skeleton> skeleton) {
		mSkeleton = std::move(skeleton);
	}
	
	std::vector<std::unique_ptr<SkinnedMeshData>>& GetSkinnedMeshData() { return mSkinnedMeshes; }
	
	void SetSkinnedMeshData( std::vector<std::unique_ptr<SkinnedMeshData>>&& meshData) {
		
		mSkinnedMeshes = std::move(meshData);
	}
	
	std::vector<std::unique_ptr<Animation>>& GetAnimationData() { return mAnimations; }
	
	void AddAnimationData(std::unique_ptr<Animation> animation) {
		mAnimations.push_back(std::move(animation));
	}
	
	void TryBuildSkeleton();
	
	void TryImportAnimations();
	
private:
	std::string GetBoneNameById(int boneId) const;
	int GetBoneIdByName(const std::string& boneName) const;
	
	void ProcessBoneAndParents(const std::shared_ptr<sfbx::LimbNode>& bone);
	
	void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) override;
	
	std::unordered_map<std::string, int> mBoneMapping;
	
	std::unique_ptr<Skeleton> mSkeleton;
	std::vector<std::unique_ptr<SkinnedMeshData>> mSkinnedMeshes;
	
	std::vector<std::unique_ptr<Animation>> mAnimations;
	
	std::vector<BoneHierarchyInfo> mBoneHierarchy; // hierarchy info
	
};
