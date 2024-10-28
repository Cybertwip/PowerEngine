#pragma once

#include "import/Fbx.hpp"

#include "animation/Transform.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <fbxsdk/core/math/fbxaffinematrix.h>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>


class SkinnedFbx : public Fbx {
public:
	SkinnedFbx() = default;
	~SkinnedFbx() = default;
	
	const std::unique_ptr<Skeleton>& GetSkeleton() {
		return mSkeleton;
	}
	
	void SetSkeleton(std::unique_ptr<Skeleton> skeleton) {
		mSkeleton = std::move(skeleton);
	}
	
	std::vector<std::unique_ptr<SkinnedMeshData>>& GetSkinnedMeshData() {
		return mSkinnedMeshes;
	}
	
	void SetSkinnedMeshData(std::vector<std::unique_ptr<SkinnedMeshData>>&& meshData) {
		mSkinnedMeshes = std::move(meshData);
	}
	
	std::vector<std::unique_ptr<Animation>>& GetAnimationData() {
		return mAnimations;
	}
	
	void AddAnimationData(std::unique_ptr<Animation> animation) {
		mAnimations.push_back(std::move(animation));
	}
	
	void TryBuildSkeleton();
	
	void TryImportAnimations();
	
protected:
	std::string GetBoneNameById(int boneId) const;
	int GetBoneIdByName(const std::string& boneName) const;
	
	void ProcessBoneAndParents(fbxsdk::FbxNode* boneNode, const std::unordered_map<std::string, fbxsdk::FbxAMatrix>& boneOffsetMatrices);
	
	void ProcessBones(fbxsdk::FbxMesh* mesh) override;
	
private:
	struct BoneHierarchyInfo {
		int bone_id;
		glm::mat4 offset;
		glm::mat4 bindpose;
		fbxsdk::FbxNode* node;
	};
	
	std::unordered_map<std::string, int> mBoneMapping;
	std::unordered_map<std::string, fbxsdk::FbxNode*> mBoneNodes;
	
	std::unique_ptr<Skeleton> mSkeleton;
	std::vector<std::unique_ptr<SkinnedMeshData>> mSkinnedMeshes;
	
	std::vector<std::unique_ptr<Animation>> mAnimations;
	
	std::vector<BoneHierarchyInfo> mBoneHierarchy;
};
