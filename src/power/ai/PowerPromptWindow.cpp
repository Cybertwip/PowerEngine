#include "PowerPromptWindow.hpp"

#include "ShaderManager.hpp"
#include "OpenAiApiClient.hpp"

#include "filesystem/ImageUtils.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"
#include "ui/ResourcesPanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include <json/json.h>

#include <iostream>
#include <filesystem>
#include <future>
#include <mutex>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>

// Include httplib
#include "httplib.h"

namespace fs = std::filesystem;

PowerPromptWindow::PowerPromptWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, PowerAi& powerAi, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent), mResourcesPanel(resourcesPanel), mPowerAi(powerAi), mRenderPass(renderpass) {
	
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

	mImageView->set_visible(false);

	mPreviewCanvas = std::make_shared<SharedSelfContainedMeshCanvas>(*mImageContainer, parent);
	
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(300, 300));
	mPreviewCanvas->set_background_color(nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
	
	mPreviewCanvas->set_visible(true);
	
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
	mGenerateButton->set_tooltip("Generate an image or 3d model based on the prompt");
	
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

void PowerPromptWindow::ProcessEvents() {
	// Currently, no additional event processing is required
}

void PowerPromptWindow::SubmitPromptAsync() {
	// Disable Generate Button to prevent multiple submissions
	mGenerateButton->set_enabled(false);
	
	// Disable Save Button since a new submission is starting
	mSaveButton->set_enabled(false);
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Generating image...");
	}
	
	std::string prompt_setup = "Analyze this prompt and return a json with boolean fields generate_image, generate_model, generate_rig, generate_animation, prompt_description, based on its analysis, only set generate_model to true if the prompt includes the word '3d', if the word '3d' is included and generate_animation is true, then generate_rig must also be true, if the prompt does not include the word '3d' then generate_rig must be false, if the prompt does not include the word 'rig', then generate_rig must be false, if the prompt does not include the word '3d' but generate_animation is true, then generate_image must be true and generate_model and generate_rig must be false, parse the prompt and fill prompt_description with the description of the prompt, if nothing matches, all the fields must be set to false and prompt_description must be an empty string, reply only with the json. Prompt: ";
	
	std::string prompt = mInputTextBox->value();
	
	prompt_setup += prompt;
	
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
	
	mPowerAi.generate_text_async(prompt_setup, [this](const std::string& json_string, const std::string& error) {
		if (!error.empty()) {
			// Handle error from OpenAI
			std::cerr << "OpenAI API Error: " << error << std::endl;
			nanogui::async([this, error]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: OpenAI API Error.");
				mGenerateButton->set_enabled(true);
			});
			return;
		}
		
		// Use regex to extract JSON from the response
		// Updated regex pattern without 'dotall' flag
		// This pattern matches the first { and the last } in the string, capturing everything in between, including newlines
		std::regex json_regex(R"((\{[\s\S]*\}))", std::regex_constants::ECMAScript);
		std::smatch matches;
		std::string extracted_json;
		
		if (std::regex_search(json_string, matches, json_regex)) {
			extracted_json = matches[1];
			std::cout << "Extracted JSON: " << extracted_json << std::endl;
		} else {
			// If regex fails, assume the entire string is JSON (fallback)
			std::cerr << "Regex failed to extract JSON. Attempting to parse the entire response." << std::endl;
			extracted_json = json_string;
		}
		
		// Parse JSON using json-cpp
		Json::CharReaderBuilder readerBuilder;
		Json::Value root;
		std::string errs;
		
		std::istringstream ss(extracted_json);
		if (!Json::parseFromStream(readerBuilder, ss, &root, &errs)) {
			// Parsing failed
			std::cerr << "JSON Parsing Error: " << errs << std::endl;
			nanogui::async([this, errs]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Failed to parse JSON response.");
				mGenerateButton->set_enabled(true);
			});
			return;
		}
		
		// Extract fields with default values
		bool generate_image = false;
		bool generate_model = false;
		bool generate_rig = false;
		bool generate_animation = false;
		std::string prompt_description;
		
		// Validate and extract 'generate_image'
		if (root.isMember("generate_image") && root["generate_image"].isBool()) {
			generate_image = root["generate_image"].asBool();
		} else {
			std::cerr << "JSON Parsing Warning: 'generate_image' field is missing or not a boolean." << std::endl;
		}
		
		// Validate and extract 'generate_model'
		if (root.isMember("generate_model") && root["generate_model"].isBool()) {
			generate_model = root["generate_model"].asBool();
		} else {
			std::cerr << "JSON Parsing Warning: 'generate_model' field is missing or not a boolean." << std::endl;
		}
		
		// Validate and extract 'generate_rig'
		if (root.isMember("generate_rig") && root["generate_rig"].isBool()) {
			generate_rig = root["generate_rig"].asBool();
		} else {
			std::cerr << "JSON Parsing Warning: 'generate_rig' field is missing or not a boolean." << std::endl;
		}
		
		// Validate and extract 'generate_animation'
		if (root.isMember("generate_animation") && root["generate_animation"].isBool()) {
			generate_animation = root["generate_animation"].asBool();
		} else {
			std::cerr << "JSON Parsing Warning: 'generate_animation' field is missing or not a boolean." << std::endl;
		}
		
		// Validate and extract 'prompt_description'
		if (root.isMember("prompt_description") && root["prompt_description"].isString()) {
			prompt_description = root["prompt_description"].asString();
		} else {
			std::cerr << "JSON Parsing Warning: 'prompt_description' field is missing or not a string." << std::endl;
		}
		
		// For debugging purposes, you can print the extracted values
		std::cout << "Parsed JSON:" << std::endl;
		std::cout << "generate_image: " << generate_image << std::endl;
		std::cout << "generate_model: " << generate_model << std::endl;
		std::cout << "generate_rig: " << generate_rig << std::endl;
		std::cout << "generate_animation: " << generate_animation << std::endl;
		std::cout << "prompt_description: " << prompt_description << std::endl;
		
		if (generate_image) {
			mImageView->set_visible(true);
			mPreviewCanvas->set_visible(false);
		} else if(generate_model) {
			mImageView->set_visible(false);
			mPreviewCanvas->set_visible(true);
		}
		
		perform_layout(screen().nvg_context());
		
		if (generate_image && !generate_animation) {
			// Asynchronously generate the image using OpenAiApiClient
			mPowerAi.generate_image_async(prompt_description, [this](const std::string& image_url, const std::string& error) {
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
		} else if (generate_image && generate_animation) {
			std::cerr << "Unsupported prompt." << std::endl;
			{
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Operation not implemented yet.");
			}
		} else if (generate_model && !generate_rig && !generate_animation) {
			// I'll do this later
		} else if (generate_model && generate_rig && !generate_animation) {
			// I'll do this later
		} else if (generate_model && generate_rig && generate_animation) {
			// I'll do this later
		} else {
			std::cerr << "Unsupported prompt." << std::endl;
			{
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Operation not supported or unrelated prompt.");
			}
		}
		
		// For now, we'll just update the status and re-enable the Generate button
		nanogui::async([this, prompt_description]() {
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Prompt processed successfully.");
			mGenerateButton->set_enabled(true);
			// Optionally, you can display the prompt_description somewhere in your UI
		});
	});
}

void PowerPromptWindow::SaveImageAsync() {
	if (mImageData.empty() || mLastImageUrl.empty()) {
		std::cerr << "No image data available to save." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: No image to save.");
		}
		return;
	}
	
	// Generate a unique filename using the current timestamp
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	
	// Thread-safe conversion of time
#ifdef _WIN32
	localtime_s(&tm, &now_time);
#else
	localtime_r(&now_time, &tm);
#endif
	
	std::ostringstream oss;
	oss << "generated_image_"
	<< std::put_time(&tm, "%Y%m%d_%H%M%S")
	<< ".png";
	std::string filename = oss.str();
	
	// If a file with the same timestamp exists, append a counter
	int counter = 1;
	while (fs::exists(filename)) {
		oss.str(""); // Clear the stream
		oss << "generated_image_"
		<< std::put_time(&tm, "%Y%m%d_%H%M%S")
		<< "_" << counter++
		<< ".png";
		filename = oss.str();
	}
	
	// Attempt to write the image data to the file
	if (!write_compressed_png_to_file(mImageData, filename)) {
		std::cerr << "Failed to decode image data for saving." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to decode image for saving.");
		}
		return;
	} else {
		std::cout << "Image saved successfully to " << filename << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Image saved successfully.");
		}
	}
}

