#include "graphics/drawing/MeshActorBuilder.hpp"

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

MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit) {
	
}

Actor& MeshActorBuilder::build(Actor& actor, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	// Add MetadataComponent to the actor
	actor.add_component<MetadataComponent>(actor.identifier(), std::filesystem::path(path).stem().string());
	
	// Determine file type based on extension
	std::filesystem::path filePath(path);
	std::string extension = filePath.extension().string();
	
	if (extension == ".fbx") {
		// Handle serialization path for classic .fbx files
		
		// Add ColorComponent
		auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
		
		// Create and load the model
		auto model = std::make_unique<SkinnedFbx>(path);
		model->LoadModel(); // Assuming LoadModel handles the serialization
		model->TryImportAnimations();
		
		std::unique_ptr<Drawable> drawableComponent;
		
		if (model->GetSkeleton() != nullptr) {
			// Skinned Mesh Handling
			std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData; // Corrected type to SkinnedMesh
			
			// Serialize Skeleton and Animations
			auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo>(std::move(model->GetSkeleton()));
			
			for (auto& animation : model->GetAnimationData()) {
				pdo->mAnimationData.push_back(std::move(animation));
			}
			
			// Add SkinnedAnimationComponent
			auto& skinnedComponent = actor.add_component<SkinnedAnimationComponent>(std::move(pdo));
			
			// Add PlaybackComponent
			actor.add_component<PlaybackComponent>();
			
			// Serialize each SkinnedMesh
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
			// Non-Skinned Mesh Handling
			std::vector<std::unique_ptr<Mesh>> meshComponentData;
			
			// Serialize each Mesh
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
		actor.add_component<TransformComponent>();
		actor.add_component<AnimationComponent>();
		
	} else if (extension == ".psk" || extension == ".pma") {
		// Handle deserialization path for .psk and .pma files
		
		// Add ColorComponent
		auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
		
		// Create Deserializer
		CompressedSerialization::Deserializer deserializer;
		
		// Load the serialized file
		if (!deserializer.load_from_file(path)) {
			std::cerr << "Failed to load serialized file: " << path << "\n";
			return actor;
		}
		
		// Create a SkinnedFbx model for deserialization
		auto model = std::make_unique<SkinnedFbx>();
		
		if (extension == ".psk") {
			// Deserialize Skinned Mesh Data
			
			int32_t meshDataCount;
			if (!deserializer.read_int32(meshDataCount)) {
				std::cerr << "Failed to read mesh data count from: " << path << "\n";
				return actor;
			}
			
			// Deserialize each SkinnedMeshData
			std::vector<std::unique_ptr<SkinnedMeshData>> skinnedMeshData;
			skinnedMeshData.reserve(meshDataCount);
			for (int32_t i = 0; i < meshDataCount; ++i) {
				// Deserialize number of material properties (if applicable)
				int32_t materialCount;
				if (!deserializer.read_int32(materialCount)) {
					std::cerr << "Failed to read material count for mesh " << i << "\n";
					return actor;
				}
				
				// Create and deserialize MeshData
				auto meshData = std::make_unique<SkinnedMeshData>();
				if (!meshData->deserialize(deserializer)) {
					std::cerr << "Failed to deserialize MeshData for mesh " << i << "\n";
					return actor;
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
				
				// Add SkinnedAnimationComponent
				auto& skinnedComponent = actor.add_component<SkinnedAnimationComponent>(std::move(pdo));
				
				// Add PlaybackComponent
				actor.add_component<PlaybackComponent>();
				
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
			actor.add_component<TransformComponent>();
			actor.add_component<AnimationComponent>();
			
		} else if (extension == ".pma") {
			// Deserialize Non-Skinned Mesh Data
			
			int32_t meshDataCount;
			if (!deserializer.read_int32(meshDataCount)) {
				std::cerr << "Failed to read mesh data count from: " << path << "\n";
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
			actor.add_component<TransformComponent>();
			actor.add_component<AnimationComponent>();
		} else {
			std::cerr << "Unsupported file extension: " << extension << "\n";
			return actor;
		}
		
	} else {
		std::cerr << "Unsupported file type for path: " << path << "\n";
	}
	
	return actor;
}
