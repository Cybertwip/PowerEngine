#pragma once

#include "filesystem/MeshActorImporter.hpp"

#include <entt/entt.hpp>

#include <nanogui/window.h>

#include <memory>

class MeshActorBuilder;
class IMeshBatch;
class ResourcesPanel;
class ShaderWrapper;
class ShaderManager;
class SharedSelfContainedMeshCanvas;
class ISkinnedMeshBatch;

struct BatchUnit;

namespace nanogui {
class CheckBox;
class RenderPass;
}

class ImportWindow : public nanogui::Window {
public:
	ImportWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void Preview(const std::string& path, const std::string& directory);

	void ProcessEvents();
	
private:
	void ImportIntoProject();
	
	ResourcesPanel& mResourcesPanel;
	
	std::unique_ptr<IMeshBatch> mMeshBatch;
	std::unique_ptr<ISkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::unique_ptr<BatchUnit> mBatchUnit;

	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;

	SharedSelfContainedMeshCanvas* mPreviewCanvas;

	nanogui::CheckBox* mMeshCheckbox;
	nanogui::CheckBox* mAnimationsCheckbox;

	entt::registry mDummyRegistry;

	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mCompressedMeshData;
};
