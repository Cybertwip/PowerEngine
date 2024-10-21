#pragma once

#include "animation/AnimationTimeProvider.hpp"

#include "filesystem/MeshActorImporter.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include <entt/entt.hpp>

#include <nanogui/window.h>
#include <nanogui/label.h> // Added for nanogui::Label

#include <memory>
#include <string>
#include <mutex>
#include <optional>

// Forward Declarations
class DeepMotionApiClient;
class MeshActorBuilder;
class MeshActorExporter;
class IMeshBatch;
class ResourcesPanel;
class ShaderWrapper;
class ShaderManager;
class SharedSelfContainedMeshCanvas;
class ISkinnedMeshBatch;

struct BatchUnit;

namespace nanogui {
class Button;
class CheckBox;
class RenderPass;
class TextBox;
}

class DallEPromptWindow : public nanogui::Window {
public:
	DallEPromptWindow(nanogui::Screen& parent, nanogui::Screen& screen, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void Preview(const std::string& path, const std::string& directory);
		
	void ProcessEvents();
	
private:	
	// Asynchronous Methods
	void SubmitPromptAsync();
	void ImportIntoProjectAsync();
	void PollJobStatusAsync(const std::string& request_id);
	
	// Existing Private Methods
	void SubmitPrompt(); // May be removed if entirely replaced by SubmitPromptAsync
	void ImportIntoProject(); // May be removed if entirely replaced by ImportIntoProjectAsync
	std::string GenerateUniqueModelName(const std::string& hash_id0, const std::string& hash_id1);
	
	ResourcesPanel& mResourcesPanel;
	
	std::unique_ptr<IMeshBatch> mMeshBatch;
	std::unique_ptr<ISkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::unique_ptr<BatchUnit> mBatchUnit;

	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	
	std::shared_ptr<SharedSelfContainedMeshCanvas> mPreviewCanvas;
	
	std::shared_ptr<nanogui::CheckBox> mMeshCheckbox;
	std::shared_ptr<nanogui::CheckBox> mAnimationsCheckbox;
	
	entt::registry mDummyRegistry;
	
	AnimationTimeProvider mDummyAnimationTimeProvider;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mCompressedMeshData;
	
	std::shared_ptr<nanogui::Label> mInputLabel;
	std::shared_ptr<nanogui::TextBox> mInputTextBox;
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Button> mSubmitButton;
	std::shared_ptr<nanogui::Button> mImportButton; // New Import button member

	std::shared_ptr<nanogui::Widget> mInputPanel; // New Import button member
	std::shared_ptr<nanogui::Widget> mImportPanel; // New Import button member

	DeepMotionApiClient& mDeepMotionApiClient;
	
	// Status Label
	std::shared_ptr<nanogui::Label> mStatusLabel;
	std::mutex mStatusMutex;
	
	std::string mActorPath;
	std::string mOutputDirectory;
	uint64_t mHashId[2] = { 0, 0 };
	
	std::unique_ptr<MeshActorExporter> mMeshActorExporter;
	
	std::optional<std::unique_ptr<CompressedSerialization::Serializer>> mSerializedPrompt;
	
	nanogui::RenderPass& mRenderPass;
	
	std::shared_ptr<Actor> mActiveActor;
};
