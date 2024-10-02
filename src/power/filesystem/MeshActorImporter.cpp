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

std::unique_ptr<MeshActorImporter::CompressedMeshActor> MeshActorImporter::process(const std::string& path, const std::string& destination) {
	
	auto actor = std::make_unique<CompressedMeshActor>();
	
	auto meshSerializer = std::make_unique<CompressedSerialization::Serializer>();

	auto model = std::make_unique<SkinnedFbx>();
		
	model->LoadModel(path);
	model->TryImportAnimations();

	// Ensure destination path exists
	std::filesystem::path destPath(destination);

	if (model->GetSkeleton() != nullptr) {
		
		meshSerializer->write_int32(model->GetSkinnedMeshData().size());
		
		int meshDataIndex = 0;
		for (auto& meshData : model->GetSkinnedMeshData()) {
									
			meshData->serialize(*meshSerializer);
			
			meshSerializer->write_int32(meshData->get_material_properties().size());
		
			if (!meshData->get_material_properties().empty()){
				for (auto& materialData : model->GetMaterialProperties()[meshDataIndex]) {
					materialData->serialize(*meshSerializer);
				}
			}
			
			meshDataIndex++;
		}
		
		model->GetSkeleton()->serialize(*meshSerializer);
		
		// Save skeleton to file
		std::filesystem::path basename = std::filesystem::path(path).stem();
		
		auto meshPath = destPath / (basename.string() + ".psk");
			
				
		actor->mMesh.mPrecomputedPath = meshPath.string();
		
		actor->mMesh.mSerializer = std::move(meshSerializer);
		
		if (!model->GetAnimationData().empty()) {
			actor->mAnimations = std::vector<CompressedMeshActor::CompressedAsset>();
		}

		int animationIndex = 0;

		for (auto& animation : model->GetAnimationData()) {
			auto serializer = std::make_unique<CompressedSerialization::Serializer>();

			animation->serialize(*serializer);
			
			std::stringstream ss;
			ss << std::setw(4) << std::setfill('0') << animationIndex++; // Pad the index with 4 zeros
			std::string paddedIndex = ss.str();
			
			auto animationPath = destPath / (basename.string() + "_" + paddedIndex + ".pan");
			
			actor->mAnimations->push_back({std::move(serializer), animationPath.string()});
		}
	} else {
		meshSerializer->write_int32(model->GetMeshData().size());
		
		int meshDataIndex = 0;
		for (auto& meshData : model->GetMeshData()) {
			
			meshData->serialize(*meshSerializer);
			
			meshSerializer->write_int32(meshData->get_material_properties().size());
			
			if (!meshData->get_material_properties().empty()){
				for (auto& materialData : model->GetMaterialProperties()[meshDataIndex]) {
					materialData->serialize(*meshSerializer);
				}
			}
			
			meshDataIndex++;
		}

		// Save mesh to file
		std::filesystem::path basename = std::filesystem::path(path).stem();
		
		auto meshPath = destPath / (basename.string() + ".pma");
				
		actor->mMesh.mPrecomputedPath = meshPath.string();
		
		actor->mMesh.mSerializer = std::move(meshSerializer);
	}
	
	return std::move(actor);
}


std::unique_ptr<MeshActorImporter::CompressedMeshActor> MeshActorImporter::process(std::stringstream& data, const std::string& modelName, const std::string& destination) {
	
	auto actor = std::make_unique<CompressedMeshActor>();
	
	auto meshSerializer = std::make_unique<CompressedSerialization::Serializer>();
	
	auto model = std::make_unique<SkinnedFbx>();
	
	model->LoadModel(data);
	model->TryImportAnimations();
	
	// Ensure destination path exists
	std::filesystem::path destPath(destination);
	
	if (model->GetSkeleton() != nullptr) {
		
		meshSerializer->write_int32(model->GetSkinnedMeshData().size());
		
		int meshDataIndex = 0;
		for (auto& meshData : model->GetSkinnedMeshData()) {
			
			meshData->serialize(*meshSerializer);
			
			meshSerializer->write_int32(meshData->get_material_properties().size());
			
			if (!meshData->get_material_properties().empty()){
				for (auto& materialData : model->GetMaterialProperties()[meshDataIndex]) {
					materialData->serialize(*meshSerializer);
				}
			}
			
			meshDataIndex++;
		}
		
		model->GetSkeleton()->serialize(*meshSerializer);
		
		// Save skeleton to file
		
		auto meshPath = destPath / (modelName + ".psk");
		
		
		actor->mMesh.mPrecomputedPath = meshPath.string();
		
		actor->mMesh.mSerializer = std::move(meshSerializer);
		
		if (!model->GetAnimationData().empty()) {
			actor->mAnimations = std::vector<CompressedMeshActor::CompressedAsset>();
		}
		
		int animationIndex = 0;
		
		for (auto& animation : model->GetAnimationData()) {
			auto serializer = std::make_unique<CompressedSerialization::Serializer>();
			
			animation->serialize(*serializer);
			
			std::stringstream ss;
			ss << std::setw(4) << std::setfill('0') << animationIndex++; // Pad the index with 4 zeros
			std::string paddedIndex = ss.str();
			
			auto animationPath = destPath / (modelName + "_" + paddedIndex + ".pan");
			
			actor->mAnimations->push_back({std::move(serializer), animationPath.string()});
		}
	} else {
		meshSerializer->write_int32(model->GetMeshData().size());
		
		int meshDataIndex = 0;
		for (auto& meshData : model->GetMeshData()) {
			
			meshData->serialize(*meshSerializer);
			
			meshSerializer->write_int32(meshData->get_material_properties().size());
			
			if (!meshData->get_material_properties().empty()){
				for (auto& materialData : model->GetMaterialProperties()[meshDataIndex]) {
					materialData->serialize(*meshSerializer);
				}
			}
			
			meshDataIndex++;
		}
		
		// Save mesh to file
		auto meshPath = destPath / (modelName + ".pma");
		
		actor->mMesh.mPrecomputedPath = meshPath.string();
		
		actor->mMesh.mSerializer = std::move(meshSerializer);
	}
	
	return std::move(actor);
}

