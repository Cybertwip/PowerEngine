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
#include "components/TransformComponent.hpp"
#include "components/TransformAnimationComponent.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/IMeshBatch.hpp"
#include "graphics/drawing/ISkinnedMeshBatch.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "import/SkinnedFbx.hpp"

#include <filesystem>


MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit), mMeshActorImporter(std::make_unique<MeshActorImporter>()) {
}

Actor& MeshActorBuilder::build_mesh(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	// Deserialize Non-Skinned Mesh Data
	
	// Add MetadataComponent to the actor
	actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	
	// Add ColorComponent
	auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
	
	// Corrected: Use a non-skinned model type
	auto model = std::make_unique<Fbx>(); 
	
	int32_t meshDataCount;
	
	if (!deserializer.read_int32(meshDataCount)) {
		std::cerr << "Failed to read mesh data count" << "\n";
		return actor;
	}
	
	// Deserialize each MeshData
	std::vector<std::unique_ptr<MeshData>> meshData;
	meshData.reserve(meshDataCount);
	for (int32_t i = 0; i < meshDataCount; ++i) {
		auto singleMeshData = std::make_unique<MeshData>();
		if (!singleMeshData->deserialize(deserializer)) {
			std::cerr << "Failed to deserialize MeshData for mesh " << i << "\n";
			return actor;
		}
		
		singleMeshData->get_material_properties().clear();
		
		// Deserialize number of material properties (if applicable)
		int32_t materialCount;
		if (!deserializer.read_int32(materialCount)) {
			std::cerr << "Failed to read material count for mesh " << i << "\n";
			return actor;
		}
		
		for (int32_t i = 0; i < materialCount; ++i) {
			SerializableMaterialProperties material;
			
			material.deserialize(deserializer);
			
			auto properties = std::make_shared<MaterialProperties>();
			
			properties->deserialize(material);
			
			singleMeshData->get_material_properties().push_back(properties);
		}
		
		meshData.push_back(std::move(singleMeshData));
	}
	
	// Set MeshData in the model
	model->SetMeshData(std::move(meshData));
	
	// Now, set up the Actor's components based on deserialized data
	std::unique_ptr<Drawable> drawableComponent;
	
	// Non-Skinned Mesh Handling
	std::vector<std::unique_ptr<Mesh>> meshComponentData;
	
	for (auto& meshDataItem : model->GetMeshData()) {
		meshComponentData.push_back(std::make_unique<Mesh>(
														   std::move(meshDataItem),
														   meshShader,
														   mBatchUnit.mMeshBatch,
														   colorComponent
														   ));
	}
	
	// Create MeshComponent
	drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(model));
	
	// Add DrawableComponent
	actor.add_component<DrawableComponent>(std::move(drawableComponent));
	
	// Add TransformComponent and AnimationComponent
	auto& transform = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transform, timeProvider);
	
	return actor;
}

Actor& MeshActorBuilder::build_skinned(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName,  CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader){
	
	// Add MetadataComponent to the actor
	actor.add_component<MetadataComponent>(actor.identifier(), actorName);
	
	// Add ColorComponent
	auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
	
	// Create a SkinnedFbx model for deserialization
	auto model = std::make_unique<SkinnedFbx>();
	
	int32_t meshDataCount;
	if (!deserializer.read_int32(meshDataCount)) {
		std::cerr << "Failed to read mesh data count" << "\n";
		return actor;
	}
	
	// Deserialize each SkinnedMeshData
	std::vector<std::unique_ptr<SkinnedMeshData>> skinnedMeshData;
	skinnedMeshData.reserve(meshDataCount);
	for (int32_t i = 0; i < meshDataCount; ++i) {
		// Create and deserialize MeshData
		auto meshData = std::make_unique<SkinnedMeshData>();
		if (!meshData->deserialize(deserializer)) {
			std::cerr << "Failed to deserialize MeshData for mesh " << i << "\n";
			return actor;
		}
		
		
		meshData->get_material_properties().clear();
		
		// Deserialize number of material properties (if applicable)
		int32_t materialCount;
		if (!deserializer.read_int32(materialCount)) {
			std::cerr << "Failed to read material count for mesh " << i << "\n";
			return actor;
		}
		
		for (int32_t i = 0; i < materialCount; ++i) {
			SerializableMaterialProperties material;
			
			material.deserialize(deserializer);
			
			auto properties = std::make_shared<MaterialProperties>();
			
			properties->deserialize(material);
			
			meshData->get_material_properties().push_back(properties);
		}
		
		skinnedMeshData.push_back(std::move(meshData));
	}
	
	// Set SkinnedMeshData in the model
	model->SetSkinnedMeshData(std::move(skinnedMeshData));
	
	// Deserialize Skeleton
	auto skeleton = std::make_unique<Skeleton>();
	if (!skeleton->deserialize(deserializer)) {
		std::cerr << "Failed to deserialize Skeleton.\n";
		return actor;
	}
	model->SetSkeleton(std::move(skeleton));
	
	// Now, set up the Actor's components based on deserialized data
	std::unique_ptr<Drawable> drawableComponent;
	
	if (model->GetSkeleton() != nullptr) {
		// Skinned Mesh Handling
		std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData; // Corrected type to SkinnedMesh
		
		// Create SkinnedAnimationPdo with deserialized skeleton
		auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo>(std::move(model->GetSkeleton()));
		
		for (auto& animation : model->GetAnimationData()) {
			pdo->mAnimationData.push_back(std::move(animation));
		}
		
		// Add PlaybackComponent
		auto& plabackComponent = actor.add_component<PlaybackComponent>();

		// Add SkinnedAnimationComponent
		auto& skinnedComponent = actor.add_component<SkinnedAnimationComponent>(std::move(pdo), plabackComponent, timeProvider);
				
		// Create SkinnedMesh instances from deserialized data
		for (auto& skinnedMeshData : model->GetSkinnedMeshData()) {
			skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(
																			 std::move(skinnedMeshData),
																			 skinnedShader,
																			 mBatchUnit.mSkinnedMeshBatch,
																			 colorComponent,
																			 skinnedComponent
																			 ));
		}
		
		// Create SkinnedMeshComponent
		drawableComponent = std::make_unique<SkinnedMeshComponent>(std::move(skinnedMeshComponentData), std::move(model));
	} else {
		// Non-Skinned Mesh Handling (unlikely for .psk but handled for completeness)
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshData : model->GetMeshData()) {
			meshComponentData.push_back(std::make_unique<Mesh>(
															   std::move(meshData),
															   meshShader,
															   mBatchUnit.mMeshBatch,
															   colorComponent
															   ));
		}
		
		// Create MeshComponent
		drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(model));
	}
	
	// Add DrawableComponent
	actor.add_component<DrawableComponent>(std::move(drawableComponent));
	
	// Add TransformComponent and AnimationComponent
	auto& transform = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transform, timeProvider);
	
	return actor;
}

Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	// Determine file type based on extension
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(path).stem().string();
	
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;

	if (extension == ".fbx") {
		auto compressedMeshActor = mMeshActorImporter->process(path, "./dummyDestination/");
		
		std::stringstream compressedData;
		
		compressedMeshActor->mMesh.mSerializer->get_compressed_data(compressedData);
				
		if (!deserializer.initialize(compressedData)) {
			std::cerr << "Failed to load serialized file: " << path << "\n";
			return actor;
		}
		
		
		filePath = std::filesystem::path(compressedMeshActor->mMesh.mPrecomputedPath);
		extension = filePath.extension().string();

	} else {
		// Load the serialized file
		if (!deserializer.load_from_file(path)) {
			std::cerr << "Failed to load serialized file: " << path << "\n";
			return actor;
		}
		
	}
	
	if (extension == ".psk") {
		return build_skinned(actor, timeProvider, actorName, deserializer, meshShader, skinnedShader);
	} else if (extension == ".pma") {
		return build_mesh(actor, timeProvider, actorName, deserializer, meshShader, skinnedShader);
	} else {
		std::cerr << "Unsupported file extension: " << extension << "\n";
		return actor;
	}
	
	return actor;
}
