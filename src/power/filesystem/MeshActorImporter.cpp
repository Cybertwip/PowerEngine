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
	
	CompressedSerialization::Serializer meshSerializer;

	auto model = std::make_unique<SkinnedFbx>(path);
		
	model->LoadModel();
	model->TryImportAnimations();

	// Ensure destination path exists
	std::filesystem::path destPath(destination);
	if (!std::filesystem::exists(destPath)) {
		std::filesystem::create_directories(destPath);
	}

	if (model->GetSkeleton() != nullptr) {
		
		meshSerializer.write_int32(model->GetSkinnedMeshData().size());
		
		for (auto& meshData : model->GetSkinnedMeshData()) {
			
			meshSerializer.write_int32(meshData->get_material_properties().size());
			
			meshData->serialize(meshSerializer);
		}
		
		model->GetSkeleton()->serialize(meshSerializer);
		
		// Save skeleton to file
		std::filesystem::path basename = std::filesystem::path(path).stem();
		meshSerializer.save_to_file(destPath / (basename.string() + ".psk"));

		int animationIndex = 0;

		for (auto& animation : model->GetAnimationData()) {
			CompressedSerialization::Serializer serializer;
			animation->serialize(serializer);
			
			std::stringstream ss;
			ss << std::setw(4) << std::setfill('0') << animationIndex++; // Pad the index with 4 zeros
			std::string paddedIndex = ss.str();
			serializer.save_to_file(destPath / (basename.string() + "_" + paddedIndex + ".pan"));
		}
	} else {
		meshSerializer.write_int32(model->GetMeshData().size());
		
		for (auto& meshData : model->GetMeshData()) {
			meshData->serialize(meshSerializer);
		}

		// Save mesh to file
		std::filesystem::path basename = std::filesystem::path(path).stem();
		meshSerializer.save_to_file(destPath / (basename.string() + ".pma"));
	}
}
