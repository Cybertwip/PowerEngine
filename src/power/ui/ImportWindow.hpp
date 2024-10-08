#pragma once

#include "animation/AnimationTimeProvider.hpp"

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
	ImportWindow(nanogui::Screen& parent, nanogui::Screen& screen, ResourcesPanel& resourcesPanel, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
		
	void Preview(const std::string& path, const std::string& directory);

	void ProcessEvents();
	
private:
	void ImportIntoProject();
	
	std::shared_ptr<ResourcesPanel> mResourcesPanel;
	
	std::unique_ptr<IMeshBatch> mMeshBatch;
	std::unique_ptr<ISkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::unique_ptr<BatchUnit> mBatchUnit;

	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;

	std::shared_ptr<SharedSelfContainedMeshCanvas> mPreviewCanvas;

	std::shared_ptr<nanogui::CheckBox> mMeshCheckbox;
	std::shared_ptr<nanogui::CheckBox> mAnimationsCheckbox;
	
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Button> mImportButton;
	std::shared_ptr<nanogui::Widget> mCheckboxPanel;

	nanogui::RenderPass& mRenderPass;
	
	entt::registry mDummyRegistry;
	AnimationTimeProvider mDummyAnimationTimeProvider;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mCompressedMeshData;
};
