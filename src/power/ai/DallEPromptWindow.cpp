#include "DallEPromptWindow.hpp"

#include "ShaderManager.hpp"
#include "DallEApiClient.hpp"

#include "filesystem/ImageUtils.hpp"
#include "ui/ResourcesPanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include <iostream>
#include <filesystem>
#include <future>
#include <mutex>
#include <sstream>
#include <fstream>

// Include httplib
#include "httplib.h"

namespace fs = std::filesystem;

DallEPromptWindow::DallEPromptWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, DallEApiClient& dallEApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent), mResourcesPanel(resourcesPanel), mDallEApiClient(dallEApiClient), mRenderPass(renderpass) {
	
	set_fixed_size(nanogui::Vector2i(600, 600)); // Adjusted size for better layout
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	set_title("DALL·E Image Generation");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(30, 30));
	mCloseButton->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
	});
	
	
	// **New Centering Container for ImageView**
	mImageContainer = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	
	mImageContainer->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 0));
	
	// Image View
	mImageView = std::make_shared<nanogui::ImageView>(*mImageContainer, parent); // Assuming ImageView can take an empty string initially
	mImageView->set_fixed_size(nanogui::Vector2i(300, 300));
	mImageView->set_background_color(nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
	
	// Input Panel for Prompt
	mInputPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mInputPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 5, 5));
	
	mInputLabel = std::make_shared<nanogui::Label>(*mInputPanel, "Enter Image Generation Prompt:", "sans-bold");
	mInputLabel->set_fixed_height(25);
	
	mInputTextBox = std::make_shared<nanogui::TextBox>(*mInputPanel, "");
	mInputTextBox->set_fixed_size(nanogui::Vector2i(580, 100));
	mInputTextBox->set_alignment(nanogui::TextBox::Alignment::Left);
	mInputTextBox->set_placeholder("e.g., A futuristic cityscape at sunset");
	mInputTextBox->set_font_size(14);
	mInputTextBox->set_editable(true);
	
	// Buttons Panel
	mButtonsPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mButtonsPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 0));
	
	mGenerateButton = std::make_shared<nanogui::Button>(*mButtonsPanel, "Generate");
	mGenerateButton->set_fixed_size(nanogui::Vector2i(100, 40));
	mGenerateButton->set_callback([this]() {
		nanogui::async([this]() { this->SubmitPromptAsync(); });
	});
	mGenerateButton->set_tooltip("Generate an image based on the prompt");
	
	mSaveButton = std::make_shared<nanogui::Button>(*mButtonsPanel, "Save");
	mSaveButton->set_fixed_size(nanogui::Vector2i(100, 40));
	mSaveButton->set_enabled(false);
	mSaveButton->set_callback([this]() {
		nanogui::async([this]() { this->SaveImageAsync(); });
	});
	mSaveButton->set_tooltip("Save the generated image");

	// Status Label
	mStatusLabel = std::make_shared<nanogui::Label>(*this, "Status: Idle", "sans-bold");
	mStatusLabel->set_fixed_height(20);
	mStatusLabel->set_font_size(14);
	
	// Initialize the download client as nullptr
	mDownloadClient = nullptr;
}

void DallEPromptWindow::ProcessEvents() {
	// Currently, no additional event processing is required
}

void DallEPromptWindow::SubmitPromptAsync() {
	// Disable Generate Button to prevent multiple submissions
	mGenerateButton->set_enabled(false);
	
	// Disable Save Button since a new submission is starting
	mSaveButton->set_enabled(false);
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Generating image...");
	}
	
	std::string prompt = mInputTextBox->value();
	if (prompt.empty()) {
		// Show error message to the user
		std::cerr << "Prompt is empty. Please enter a valid prompt." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Prompt is empty.");
		}
		mGenerateButton->set_enabled(true);
		return;
	}
	
	// Asynchronously generate the image using DallEApiClient
	mDallEApiClient.generate_image_async(prompt, [this](const std::string& image_url, const std::string& error) {
		if (!error.empty()) {
			std::cerr << "Failed to generate image: " << error << std::endl;
			nanogui::async([this]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Failed to generate image.");
				mGenerateButton->set_enabled(true);
			});
			return;
		}
		
		std::cout << "Image generated successfully. URL: " << image_url << std::endl;
		mLastImageUrl = image_url;
		
		// Parse the URL to get host and path
		std::string protocol, host, path;
		std::string::size_type protocol_pos = image_url.find("://");
		if (protocol_pos != std::string::npos) {
			protocol = image_url.substr(0, protocol_pos);
			protocol_pos += 3;
		} else {
			std::cerr << "Invalid image URL format: " << image_url << std::endl;
			nanogui::async([this]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Invalid image URL.");
				mGenerateButton->set_enabled(true);
			});
			return;
		}
		
		std::string::size_type host_pos = image_url.find("/", protocol_pos);
		if (host_pos != std::string::npos) {
			host = image_url.substr(protocol_pos, host_pos - protocol_pos);
			path = image_url.substr(host_pos);
		} else {
			host = image_url.substr(protocol_pos);
			path = "/";
		}
		
		// Determine port
		int port = 443; // Default HTTPS
		if (protocol == "http") {
			port = 80;
		}
		
		// Initialize the download client if it's not already initialized or if the host has changed
		mDownloadClient = std::make_unique<httplib::SSLClient>(host.c_str(), port);
		mDownloadClient->set_follow_location(true);
		// Optionally, set timeouts or other settings here

		// Perform the GET request to download the image
		auto res_download = mDownloadClient->Get(path.c_str());
		if (res_download && res_download->status == 200) {
			// Load image data into mImageData
			mImageData.assign(res_download->body.begin(), res_download->body.end());
			
			if (!mImageData.empty()) {
				nanogui::async([this]() {
					// Update ImageView with the new texture
					mImageView->set_image(std::make_shared<nanogui::Texture>(mImageData.data(), mImageData.size(),
																			 nanogui::Texture::InterpolationMode::Bilinear,
																			 nanogui::Texture::InterpolationMode::Nearest,
																			 nanogui::Texture::WrapMode::Repeat));
					

					
					mImageView->image()->resize(nanogui::Vector2i(600, 600));
					
					// Update Status
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Image generated successfully.");
					
					// Enable Save Button
					mSaveButton->set_enabled(true);
					
					// Re-enable Generate Button
					mGenerateButton->set_enabled(true);
				});
			} else {
				std::cerr << "Failed to decode image data." << std::endl;
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Failed to decode image.");
					mGenerateButton->set_enabled(true);
				});
			}
		} else {
			std::cerr << "Failed to download image. HTTP Status: "
			<< (res_download ? std::to_string(res_download->status) : "No Response") << std::endl;
			nanogui::async([this]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Failed to download image.");
				mGenerateButton->set_enabled(true);
			});
		}
	});
}

void DallEPromptWindow::SaveImageAsync() {
	if (mImageData.empty() || mLastImageUrl.empty()) {
		std::cerr << "No image data available to save." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: No image to save.");
		}
		return;
	}
	
	// Determine a unique filename
	std::string filename = "generated_image.png"; // You can enhance this to generate unique filenames
	int counter = 1;
	while (fs::exists(filename)) {
		filename = "generated_image_" + std::to_string(counter++) + ".png";
	}
	if (!write_compressed_png_to_file(mImageData, filename)) {
		std::cerr << "Failed to decode image data for saving." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to decode image for saving.");
		}
		return;
	} else {
		std::cerr << "Failed to save image to " << filename << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to save image.");
		}
	}
}
