#include "graphics/drawing/MeshActorBuilder.hpp"

#include "actors/Actor.hpp"
#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/ModelMetadataComponent.hpp"
#include "components/PlaybackComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/TransformAnimationComponent.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/IMeshBatch.hpp"
#include "graphics/drawing/ISkinnedMeshBatch.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"
#include "graphics/shading/MeshData.hpp"
#include "import/ModelImporter.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit) {
	// The MeshActorImporter member is removed, so no initialization is needed.
}

Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	std::filesystem::path filePath(path);
	std::string actorName = filePath.stem().string();
	
	// Use the new ModelImporter directly.
	auto importer = std::make_unique<ModelImporter>();
	if (!importer->LoadModel(path)) {
		std::cerr << "Failed to process model file with ModelImporter: " << path << "\n";
		return actor;
	}
	
	// Pass the populated importer to the main build logic.
	return build_from_model_data(actor, timeProvider, std::move(importer), path, actorName, meshShader, skinnedShader);
}

Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, std::stringstream& dataStream, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	std::string actorName = filePath.stem().string();
	
	// Convert stringstream to vector<char> for the ModelImporter API.
	const std::string& s = dataStream.str();
	std::vector<char> dataVec(s.begin(), s.end());
	
	// Use the new ModelImporter directly.
	auto importer = std::make_unique<ModelImporter>();
	if (!importer->LoadModel(dataVec, extension)) {
		std::cerr << "Failed to process model from stream with ModelImporter: " << path << "\n";
		return actor;
	}
	
	// Pass the populated importer to the main build logic.
	return build_from_model_data(actor, timeProvider, std::move(importer), path, actorName, meshShader, skinnedShader);
}

Actor& MeshActorBuilder::build_from_model_data(Actor& actor, AnimationTimeProvider& timeProvider, std::unique_ptr<ModelImporter> importer, const std::string& path, const std::string& actorName, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	// Add common components
	auto& metadataComponent = actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	
	std::unique_ptr<Drawable> drawableComponent;
	
	// The presence of a skeleton determines if the mesh is skinned.
	if (importer->GetSkeleton()) {
		// --- Skinned Mesh Handling ---
		std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
		
		auto& skeletonComponent = actor.add_component<SkeletonComponent>(*(importer->GetSkeleton()));
		actor.add_component<SkinnedAnimationComponent>(skeletonComponent, timeProvider);
		
		for (auto& meshDataItem : importer->GetMeshData()) {
			// ModelImporter guarantees SkinnedMeshData if a skeleton is present.
			auto& skinnedMeshData = static_cast<SkinnedMeshData&>(*meshDataItem);
			skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(
																			 skinnedMeshData,
																			 skinnedShader,
																			 mBatchUnit.mSkinnedMeshBatch,
																			 metadataComponent,
																			 colorComponent,
																			 skeletonComponent
																			 ));
		}
		
		// The SkinnedMeshComponent takes ownership of the importer to keep all data alive.
		drawableComponent = std::make_unique<SkinnedMeshComponent>(std::move(skinnedMeshComponentData), std::move(importer));
		
	} else {
		// --- Non-Skinned (Static) Mesh Handling ---
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshDataItem : importer->GetMeshData()) {
			meshComponentData.push_back(std::make_unique<Mesh>(
															   *meshDataItem,
															   meshShader,
															   mBatchUnit.mMeshBatch,
															   metadataComponent,
															   colorComponent
															   ));
		}
		
		// The MeshComponent takes ownership of the importer to keep all data alive.
		drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(importer));
	}
	
	if (drawableComponent) {
		actor.add_component<DrawableComponent>(std::move(drawableComponent));
	}
	
	// Handle animations if they exist
	auto* skinnedMeshComp = dynamic_cast<SkinnedMeshComponent*>(&actor.get_component<DrawableComponent>().drawable());
	if (skinnedMeshComp) {
		auto& animations = skinnedMeshComp->get_importer().GetAnimations();
		if (!animations.empty()) {
			// Attach animation data to the SkinnedAnimationComponent if it exists
			if (actor.find_component<SkinnedAnimationComponent>()) {
				auto& playbackComponent = actor.get_component<SkinnedAnimationComponent>();
				// Use the first animation by default
				auto playbackData = std::make_shared<PlaybackData>(std::move(animations.front()));
				playbackComponent.setPlaybackData(playbackData);
			}
		}
	}
	
	
	if (actor.find_component<ModelMetadataComponent>()) {
		actor.remove_component<ModelMetadataComponent>();
	}
	
	actor.add_component<ModelMetadataComponent>(path);
	
	return actor;
}
