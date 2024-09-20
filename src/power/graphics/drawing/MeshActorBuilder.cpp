#include "graphics/drawing/MeshActorBuilder.hpp"

#include <filesystem>

#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/TransformComponent.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "import/SkinnedFbx.hpp"

MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit) {
	
}

Actor& MeshActorBuilder::build(Actor& actor, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	actor.add_component<MetadataComponent>(actor.identifier(), std::filesystem::path(path).stem().string());

	auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());

	
	if (mModels.find(path) == mModels.end()) {
		// If the model does not exist, create and store it
		mModels[path] = std::make_unique<SkinnedFbx>(path);
	} else {
		mModels[path].reset();
		mModels[path] = std::make_unique<SkinnedFbx>(path);
	}

	SkinnedFbx& model = *mModels[path];

	model.LoadModel();
	model.TryBuildSkeleton();
	model.TryImportAnimations();

	std::unique_ptr<Drawable> drawableComponent;
	
	if (model.GetSkeleton() != nullptr) {
		std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
		
		for (auto& skinnedMeshData : model.GetSkinnedMeshData()) {
			skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(std::move(skinnedMeshData), skinnedShader, mBatchUnit.mSkinnedMeshBatch, colorComponent));
		}
		
		drawableComponent = std::make_unique<SkinnedMeshComponent>(skinnedMeshComponentData);
		
		
		auto pdo = SkinnedAnimationComponent::SkinnedAnimationPdo();

		for (auto& animation : model.GetAnimationData()) {
			pdo.mAnimationData.push_back(std::ref(*animation));
		}
		
		actor.add_component<SkinnedAnimationComponent>(pdo);

	} else {
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshData : model.GetMeshData()) {
			meshComponentData.push_back(std::make_unique<Mesh>(std::move(meshData), meshShader, mBatchUnit.mMeshBatch, colorComponent));
		}
		
		drawableComponent = std::make_unique<MeshComponent>(meshComponentData);
	}

	actor.add_component<DrawableComponent>(std::move(drawableComponent));
	actor.add_component<TransformComponent>();
	actor.add_component<AnimationComponent>();

	return actor;
}
