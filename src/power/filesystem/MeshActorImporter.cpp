#include "filesystem/MeshActorImporter.hpp"

#include <filesystem>

#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/PlaybackComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/TransformComponent.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "import/SkinnedFbx.hpp"

MeshActorImporter::MeshActorImporter() {
	
}

void MeshActorImporter::process(const std::string& path, const std::string& destination) {
	
	auto model = std::make_unique<SkinnedFbx>(path);
		
	model->LoadModel();
	model->TryImportAnimations();
		
	if (model->GetSkeleton() != nullptr) {
		std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
		
		auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo> (std::move(model->GetSkeleton()));
		
		for (auto& animation : model->GetAnimationData()) {
			pdo->mAnimationData.push_back(std::move(animation));
		}
		
//		auto& skinnedComponent = actor.add_component<SkinnedAnimationComponent>(std::move(pdo));
		
//		actor.add_component<PlaybackComponent>();
		
		for (auto& skinnedMeshData : model->GetSkinnedMeshData()) {
//			skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(std::move(skinnedMeshData), skinnedShader, mBatchUnit.mSkinnedMeshBatch, colorComponent,
//				skinnedComponent));
		}
		
//		drawableComponent = std::make_unique<SkinnedMeshComponent>(skinnedMeshComponentData, std::move(model));
	} else {
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshData : model->GetMeshData()) {
//			meshComponentData.push_back(std::make_unique<Mesh>(std::move(meshData), meshShader, mBatchUnit.mMeshBatch, colorComponent));
		}
		
//		drawableComponent = std::make_unique<MeshComponent>(meshComponentData, std::move(model));
	}
	
//	actor.add_component<DrawableComponent>(std::move(drawableComponent));
//	actor.add_component<TransformComponent>();
//	actor.add_component<AnimationComponent>();
	
//	return actor;
}
