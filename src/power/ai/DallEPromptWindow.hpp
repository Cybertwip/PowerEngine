#pragma once

#include "animation/AnimationTimeProvider.hpp"

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
class DallEApiClient;
class ResourcesPanel;

namespace nanogui {
class Button;
class Label;
class ImageView;
class TextBox;
}

class DallEPromptWindow : public nanogui::Window {
public:
	DallEPromptWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, DallEApiClient& dallEApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void ProcessEvents();
	
	private:
	// Asynchronous Methods
	void SubmitPromptAsync();
	void SaveImageAsync();
	
	// UI Components
	ResourcesPanel& mResourcesPanel;
	
	std::shared_ptr<nanogui::ImageView> mImageView;
	
	std::shared_ptr<nanogui::Label> mInputLabel;
	std::shared_ptr<nanogui::TextBox> mInputTextBox;
	std::shared_ptr<nanogui::Button> mCloseButton;
	std::shared_ptr<nanogui::Button> mGenerateButton;
	std::shared_ptr<nanogui::Button> mSaveButton;
	
	DallEApiClient& mDallEApiClient;
	
	// Status Label
	std::shared_ptr<nanogui::Label> mStatusLabel;
	std::mutex mStatusMutex;
	
	// Image Data
	std::string mLastImageUrl;
	std::vector<uint8_t> mImageData; // Stores the downloaded image data
	
	// HTTP Client for downloading images
	std::unique_ptr<httplib::SSLClient> mDownloadClient; // Added member
	
	nanogui::RenderPass& mRenderPass;
};
