#pragma once

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

class PromptWindow : public nanogui::Window {
public:
	PromptWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void Preview(const std::string& path, const std::string& directory);
	
	void Preview(const std::string& path, const std::string& directory, CompressedSerialization::Deserializer& deserializer, std::optional<std::unique_ptr<SkinnedAnimationComponent::SkinnedAnimationPdo>> pdo);
	
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
	
	SharedSelfContainedMeshCanvas* mPreviewCanvas;
	
	nanogui::CheckBox* mMeshCheckbox;
	nanogui::CheckBox* mAnimationsCheckbox;
	
	entt::registry mDummyRegistry;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mCompressedMeshData;
	
	nanogui::TextBox* mInputTextBox;
	nanogui::Button* mSubmitButton;
	
	DeepMotionApiClient& mDeepMotionApiClient;
	
	// Status Label
	nanogui::Label* mStatusLabel;
	std::mutex mStatusMutex;
	
	std::string mActorPath;
	std::string mOutputDirectory;
	uint64_t mHashId[2] = { 0, 0 };
	
	std::unique_ptr<MeshActorExporter> mMeshActorExporter;
	
	std::optional<CompressedSerialization::Serializer> mSerializedPrompt;
};
