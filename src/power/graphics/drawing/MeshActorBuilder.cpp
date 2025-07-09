#include "graphics/drawing/MeshActorBuilder.hpp"

#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/PlaybackComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/TransformAnimationComponent.hpp"

#include "filesystem/MeshActorImporter.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/IMeshBatch.hpp"
#include "graphics/drawing/ISkinnedMeshBatch.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "import/SkinnedFbx.hpp"
#include "import/Fbx.hpp"

#include <filesystem>
#include <sstream>
#include <iostream>


MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit), mMeshActorImporter(std::make_unique<MeshActorImporter>()) {
}

Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	
	if (extension != ".fbx") {
		std::cerr << "Unsupported file extension: " << extension << ". Only .fbx is supported." << "\n";
		return actor;
	}
	
	std::string actorName = filePath.stem().string();
	
	// Directly process the FBX file to get the model and animation data.
	auto fbxData = mMeshActorImporter->processFbx(path);
	if (!fbxData || !fbxData->mModel) {
		std::cerr << "Failed to process FBX file: " << path << "\n";
		return actor;
	}
	
	return build_from_fbx_data(actor, timeProvider, previewTimeProvider, std::move(fbxData), path, actorName, meshShader, skinnedShader);
}



Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::stringstream& dataStream, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	
	if (extension != ".fbx") {
		std::cerr << "Unsupported file extension: " << extension << ". Only .fbx is supported." << "\n";
		return actor;
	}
	
	std::string actorName = filePath.stem().string();
	
	// Directly process the FBX file from the stream.
	auto fbxData = mMeshActorImporter->processFbx(dataStream, actorName);
	if (!fbxData || !fbxData->mModel) {
		std::cerr << "Failed to process FBX from stream: " << path << "\n";
		return actor;
	}
	
	return build_from_fbx_data(actor, timeProvider, previewTimeProvider, std::move(fbxData), path, actorName, meshShader, skinnedShader);
}

Actor& MeshActorBuilder::build_from_fbx_data(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::unique_ptr<MeshActorImporter::FbxData> fbxData, const std::string& path, const std::string& actorName, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	auto model = std::move(fbxData->mModel);
	
	// Add common components
	auto& metadataComponent = actor.add_component<MetadataComponent>(Hash32::generate_crc32_from_string(path), actorName);
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	auto& transformComponent = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);
	
	std::unique_ptr<Drawable> drawableComponent;
	
	// Check if the model is skinned by attempting a dynamic cast
	if (SkinnedFbx* skinnedModel = dynamic_cast<SkinnedFbx*>(model.get())) {
		// --- Skinned Mesh Handling ---
		if (skinnedModel->GetSkeleton()) {
			std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
			
			auto& skeletonComponent = actor.add_component<SkeletonComponent>((*skinnedModel->GetSkeleton()));
			actor.add_component<SkinnedAnimationComponent>(skeletonComponent, previewTimeProvider);
			
			for (auto& skinnedMeshData : skinnedModel->GetSkinnedMeshData()) {
				skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(
																				 *skinnedMeshData,
																				 skinnedShader,
																				 mBatchUnit.mSkinnedMeshBatch,
																				 metadataComponent,
																				 colorComponent,
																				 skeletonComponent
																				 ));
			}
			
			drawableComponent = std::make_unique<SkinnedMeshComponent>(std::move(skinnedMeshComponentData), std::move(model));
		}
	} else if (Fbx* staticModel = dynamic_cast<Fbx*>(model.get())) {
		// --- Non-Skinned Mesh Handling ---
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshDataItem : staticModel->GetMeshData()) {
			meshComponentData.push_back(std::make_unique<Mesh>(
															   *meshDataItem,
															   meshShader,
															   mBatchUnit.mMeshBatch,
															   metadataComponent,
															   colorComponent
															   ));
		}
		
		drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(model));
	}
	
	if (drawableComponent) {
		actor.add_component<DrawableComponent>(std::move(drawableComponent));
	}
	
	// Handle animations if they exist
	if (fbxData->mAnimations.has_value() && !fbxData->mAnimations->empty()) {
		// Ensure a playback component exists to receive the animation
		if (SkinnedFbx* skinnedModel = dynamic_cast<SkinnedFbx*>(model.get())) {

			if (!actor.find_component<SkinnedAnimationComponent>()) {
				actor.add_component<SkinnedAnimationComponent>((*(skinnedModel->GetSkeleton()), timeProvider));
			}
			
			auto& playbackComponent = actor.get_component<SkinnedAnimationComponent>();
			auto playbackData = std::make_shared<PlaybackData>(std::move(fbxData->mAnimations.value().front()));
			playbackComponent.setPlaybackData(playbackData);
		}
	}
	
	return actor;
}
