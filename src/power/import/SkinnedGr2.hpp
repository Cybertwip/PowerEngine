#pragma once

#include "import/Gr2.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"
#include <unordered_map>

class SkinnedGr2 : public Gr2 {
public:
	SkinnedGr2() = default;
	~SkinnedGr2() = default;
	
	const std::unique_ptr<Skeleton>& GetSkeleton();
	void SetSkeleton(std::unique_ptr<Skeleton> skeleton);
	std::vector<std::unique_ptr<SkinnedMeshData>>& GetSkinnedMeshData();
	void SetSkinnedMeshData(std::vector<std::unique_ptr<SkinnedMeshData>>&& meshData);
	std::vector<std::unique_ptr<Animation>>& GetAnimationData();
	
	void TryImportAnimations();
	
protected:
	void ProcessSkeletons(const granny_model* model, const granny_mesh* mesh, MeshData& meshData) override;
	
private:
	void BuildSkeleton(const granny_skeleton* skel);
	void ProcessAnimation(const granny_animation* grnAnim);
	
	std::unique_ptr<Skeleton> mSkeleton;
	std::vector<std::unique_ptr<SkinnedMeshData>> mSkinnedMeshes;
	std::vector<std::unique_ptr<Animation>> mAnimations;
	std::unordered_map<std::string, int> mBoneMapping;
};
