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

#include <filesystem>


MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit), mMeshActorImporter(std::make_unique<MeshActorImporter>()) {
}

Actor& MeshActorBuilder::build_mesh(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	// Deserialize Non-Skinned Mesh Data
	
	std::string crcGenerator = deserializer.get_header_string();
	
	// Add MetadataComponent to the actor
	auto& metadataComponent = actor.add_component<MetadataComponent>(Hash32::generate_crc32_from_string(crcGenerator), actorName);
	
	// Add ColorComponent
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	
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
														   *meshDataItem,
														   meshShader,
														   mBatchUnit.mMeshBatch,
														   metadataComponent,
														   colorComponent
														   ));
	}
	
	// Create MeshComponent
	drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(model));
	
	// Add DrawableComponent
	actor.add_component<DrawableComponent>(std::move(drawableComponent));

	auto& transformComponent = actor.add_component<TransformComponent>();

	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);

	return actor;
}

Actor& MeshActorBuilder::build_skinned(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& actorName,  CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader){
	
	std::string crcGenerator = deserializer.get_header_string();
	
	// Add MetadataComponent to the actor
	auto& metadataComponent = actor.add_component<MetadataComponent>(Hash32::generate_crc32_from_string(crcGenerator), actorName);

	// Add ColorComponent
	auto& colorComponent = actor.add_component<ColorComponent>(actor.identifier());
	
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
		std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
		
		auto& skeletonComponent = actor.add_component<SkeletonComponent>((*model->GetSkeleton()));
		
		actor.add_component<SkinnedAnimationComponent>( skeletonComponent, previewTimeProvider);
				
		// Create SkinnedMesh instances from deserialized data
		for (auto& skinnedMeshData : model->GetSkinnedMeshData()) {
			skinnedMeshComponentData.push_back(std::make_unique<SkinnedMesh>(
																			 *skinnedMeshData,
																			 skinnedShader,
																			 mBatchUnit.mSkinnedMeshBatch,
																			 metadataComponent,
																			 colorComponent,
																			 skeletonComponent
																			 ));
		}
		
		// Create SkinnedMeshComponent
		drawableComponent = std::make_unique<SkinnedMeshComponent>(std::move(skinnedMeshComponentData), std::move(model));
	} else {
		// Non-Skinned Mesh Handling (unlikely for .psk but handled for completeness)
		std::vector<std::unique_ptr<Mesh>> meshComponentData;
		
		for (auto& meshData : model->GetMeshData()) {
			meshComponentData.push_back(std::make_unique<Mesh>(
															   *meshData,
															   meshShader,
															   mBatchUnit.mMeshBatch,
															   metadataComponent,
															   colorComponent
															   ));
		}
		
		// Create MeshComponent
		drawableComponent = std::make_unique<MeshComponent>(std::move(meshComponentData), std::move(model));
	}
	
	// Add DrawableComponent
	actor.add_component<DrawableComponent>(std::move(drawableComponent));
	
	auto& transformComponent = actor.add_component<TransformComponent>();

	
	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);

	auto& skeletonComponent = actor.get_component<SkeletonComponent>();
	
	return actor;
}

Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	// Determine file type based on extension
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(path).stem().string();
	
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;

	if (extension == ".fbx") {
		auto compressedMeshActor = mMeshActorImporter->processFbx(path, "./dummyDestination/");
		
		std::stringstream compressedData;
		
		compressedMeshActor->mMesh.mSerializer->get_compressed_data(compressedData);
		
		uint64_t hash_id[] = { 0, 0 };
		
		Md5::generate_md5_from_compressed_data(compressedData, hash_id);
		
		// Write the unique hash identifier to the header
		compressedMeshActor->mMesh.mSerializer->write_header_raw(hash_id, sizeof(hash_id));
				

		// get the compressed data with the header included this time, - should optimize
		
		std::stringstream compressedDataHeader;

		compressedMeshActor->mMesh.mSerializer->get_compressed_data(compressedDataHeader);

		
		if (!deserializer.initialize(compressedDataHeader)) {
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
		return build_skinned(actor, timeProvider, previewTimeProvider, actorName, deserializer, meshShader, skinnedShader);
	} else if (extension == ".pma") {
		return build_mesh(actor, timeProvider, actorName, deserializer, meshShader, skinnedShader);
	} else {
		std::cerr << "Unsupported file extension: " << extension << "\n";
		return actor;
	}
	
	return actor;
}



Actor& MeshActorBuilder::build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::stringstream& dataStream, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	std::string actorName = std::filesystem::path(path).stem().string();
	
	auto testPath = std::filesystem::path(path);

	auto testExtension = testPath.extension().string();

	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> compressedMeshActor;

	if (testExtension == ".fbx") {
		compressedMeshActor = mMeshActorImporter->processFbx(dataStream, actorName, "./dummyDestination/");
	}
	
	std::stringstream compressedData;
	
	compressedMeshActor->mMesh.mSerializer->get_compressed_data(compressedData);
	
	uint64_t hash_id[] = { 0, 0 };
	
	Md5::generate_md5_from_compressed_data(compressedData, hash_id);
	
	// Write the unique hash identifier to the header
	compressedMeshActor->mMesh.mSerializer->write_header_raw(hash_id, sizeof(hash_id));
	
	
	// get the compressed data with the header included this time, - should optimize
	
	std::stringstream compressedDataHeader;
	
	compressedMeshActor->mMesh.mSerializer->get_compressed_data(compressedDataHeader);
	
	
	if (!deserializer.initialize(compressedDataHeader)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return actor;
	}
	
	
	auto filePath = std::filesystem::path(compressedMeshActor->mMesh.mPrecomputedPath);
	auto extension = filePath.extension().string();
	
	if (extension == ".psk") {
		auto& skinned_actor = build_skinned(actor, timeProvider, previewTimeProvider, actorName, deserializer, meshShader, skinnedShader);
		
		if (compressedMeshActor->mAnimations.has_value()) {
			auto serializedPrompt = std::move(compressedMeshActor->mAnimations.value()[0].mSerializer);
			
			std::stringstream animationCompressedData;
			
			serializedPrompt->get_compressed_data(animationCompressedData);
			
			CompressedSerialization::Deserializer animationDeserializer;
			
			animationDeserializer.initialize(animationCompressedData);
			
			
			auto animation = std::make_unique<Animation>();
			
			animation->deserialize(animationDeserializer);
			
			auto& playbackComponent = skinned_actor.get_component<PlaybackComponent>();

			auto playbackData = std::make_shared<PlaybackData>(std::move(animation));
			
			playbackComponent.setPlaybackData(playbackData);
		}

		
		return skinned_actor;
		
	} else if (extension == ".pma") {
		return build_mesh(actor, timeProvider, actorName, deserializer, meshShader, skinnedShader);
	} else {
		std::cerr << "Unsupported file extension: " << extension << "\n";
		return actor;
	}
	
	return actor;
}
