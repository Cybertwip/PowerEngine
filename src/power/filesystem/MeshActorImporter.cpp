#include "filesystem/MeshActorImporter.hpp"
#include "import/SkinnedFbx.hpp"
#include "animation/Animation.hpp"
#include <filesystem>
#include <sstream>
#include <utility>

MeshActorImporter::MeshActorImporter() {}

std::unique_ptr<MeshActorImporter::FbxData> MeshActorImporter::processFbx(const std::string& path) {
	auto fbxData = std::make_unique<FbxData>();
	auto model = std::make_unique<SkinnedFbx>();
	
	model->LoadModel(path);
	model->TryImportAnimations();
	
	auto& animations = model->GetAnimationData();
	if (!animations.empty()) {
		// Move the animations from the model to the FbxData container
		fbxData->mAnimations = std::move(animations);
	}
	
	fbxData->mModel = std::move(model);
	
	return fbxData;
}

std::unique_ptr<MeshActorImporter::FbxData> MeshActorImporter::processFbx(std::stringstream& data, const std::string& modelName) {
	auto fbxData = std::make_unique<FbxData>();
	auto model = std::make_unique<SkinnedFbx>();
	
	model->LoadModel(data);
	model->TryImportAnimations();
	
	auto& animations = model->GetAnimationData();
	if (!animations.empty()) {
		// Move the animations from the model to the FbxData container
		fbxData->mAnimations = std::move(animations);
	}
	
	fbxData->mModel = std::move(model);
	
	return fbxData;
}
