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
	std::shared_ptr<sfbx::Model> model;
	std::string parent_bone_name;
};



class SkinnedFbx : public Fbx {
public:
	explicit SkinnedFbx(const std::string& path);
	
	~SkinnedFbx() = default;
	
	Skeleton* GetSkeleton() const {
		return mSkeleton.get();
	}
	
	std::vector<std::unique_ptr<SkinnedMeshData>>& GetSkinnedMeshData() { return mSkinnedMeshes; }

	std::vector<std::unique_ptr<Animation>>& GetAnimationData() { return mAnimations; }

	void TryBuildSkeleton();
	
	void TryImportAnimations();
	
private:
	std::string GetBoneNameByID(int boneID) const;

	void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) override;
	
	std::unordered_map<std::string, int> mBoneMapping;
	
	std::unique_ptr<Skeleton> mSkeleton;
	std::vector<std::unique_ptr<SkinnedMeshData>> mSkinnedMeshes;
	
	std::vector<std::unique_ptr<Animation>> mAnimations;
	
	std::vector<BoneHierarchyInfo> mBoneHierarchy; // hierarchy info

};
