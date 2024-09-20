#pragma once

#include "import/Fbx.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <SmallFBX.h>

#include <ozz/base/memory/unique_ptr.h>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

class Fbx;

class SkinnedFbx : public Fbx {
public:
	explicit SkinnedFbx(const std::string& path);
	
	~SkinnedFbx() = default;
	
	ozz::animation::Skeleton* GetSkeleton() const {
		return mSkeleton.get();
	}
	
	std::vector<std::unique_ptr<SkinnedMeshData>>& GetSkinnedMeshData() { return mSkinnedMeshes; }

	std::vector<ozz::unique_ptr<ozz::animation::Animation>>& GetAnimationData() { return mAnimations; }

	void TryBuildSkeleton();
	
	void TryImportAnimations();
	
private:
	void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) override;
	
	std::unordered_map<std::string, int> mBoneMapping;
	std::vector<ozz::math::Transform> mBoneTransforms;
	ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
	
	std::vector<std::unique_ptr<SkinnedMeshData>> mSkinnedMeshes;
	
	std::vector<ozz::unique_ptr<ozz::animation::Animation>> mAnimations;
};
