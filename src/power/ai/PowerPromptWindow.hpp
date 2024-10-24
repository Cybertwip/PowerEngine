#pragma once

#include "animation/AnimationTimeProvider.hpp"

#include "filesystem/MeshActorImporter.hpp"

#include <entt/entt.hpp>

#include <nanogui/window.h>
#include <nanogui/label.h>
#include <nanogui/imageview.h> // Assuming nanogui has an ImageView class
#include <nanogui/button.h>
#include <nanogui/textbox.h>

#include <memory>
#include <string>
#include <mutex>
#include <optional>
#include <vector>

// Forward Declarations
class PowerAi;
class ResourcesPanel;
class SharedSelfContainedMeshCanvas;

namespace nanogui {
class Button;
class Label;
class ImageView;
class TextBox;
class Texture;
}

class PowerPromptWindow : public nanogui::Window {
private:
	enum EPowerMode {
		Image,
		Model
	};
	
public:
	PowerPromptWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, PowerAi& powerAi, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void ProcessEvents();
	
	private:
	// Asynchronous Methods
	void SubmitPromptAsync();
	void SaveImageAsync();
	void SaveModelAsync();
	
	// UI Components
	ResourcesPanel& mResourcesPanel;
	
	std::shared_ptr<nanogui::Widget> mInputPanel;
	std::shared_ptr<nanogui::Widget> mButtonsPanel;
	std::shared_ptr<nanogui::Widget> mImageContainer;
	
	std::shared_ptr<nanogui::Texture> mImageContent;
	std::shared_ptr<nanogui::ImageView> mImageView;

	std::shared_ptr<nanogui::Label> mInputLabel;
	std::shared_ptr<nanogui::TextBox> mInputTextBox;
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Button> mGenerateButton;
	std::shared_ptr<nanogui::Button> mSaveButton;
	
	PowerAi& mPowerAi;
	
	// Status Label
	std::shared_ptr<nanogui::Label> mStatusLabel;
	std::mutex mStatusMutex;
	
	// Image Data
	std::string mLastImageUrl;
	std::vector<uint8_t> mImageData; // Stores the downloaded image data
	
	// HTTP Client for downloading images
	std::unique_ptr<httplib::SSLClient> mDownloadClient; // Added member
	
	nanogui::RenderPass& mRenderPass;
	
	std::unique_ptr<IMeshBatch> mMeshBatch;
	std::unique_ptr<ISkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::unique_ptr<BatchUnit> mBatchUnit;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	

	std::shared_ptr<SharedSelfContainedMeshCanvas> mPreviewCanvas;
	
	entt::registry mDummyRegistry;
	
	AnimationTimeProvider mDummyAnimationTimeProvider;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mCompressedMeshData;
	
	std::string mActorPath;
	std::string mOutputDirectory;
	uint64_t mHashId[2] = { 0, 0 };
	
	std::unique_ptr<MeshActorExporter> mMeshActorExporter;
	
	std::optional<std::unique_ptr<CompressedSerialization::Serializer>> mSerializedPrompt;

	std::shared_ptr<Actor> mActiveActor;
	
	EPowerMode mPowerMode;
	
	std::unique_ptr<MeshActorImporter::CompressedMeshActor> mGeneratedMeshData;
};
