#include "PowerPromptWindow.hpp"

#include "ShaderManager.hpp"
#include "OpenAiApiClient.hpp"

#include "components/PlaybackComponent.hpp"
#include "components/TransformComponent.hpp"
#include "filesystem/ImageUtils.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/MeshActorExporter.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"
#include "graphics/drawing/SelfContainedSkinnedMeshBatch.hpp"

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
: nanogui::Window(parent), mResourcesPanel(resourcesPanel), mPowerAi(powerAi), mRenderPass(renderpass), mDummyAnimationTimeProvider(60 * 30) {
	
	set_fixed_size(nanogui::Vector2i(600, 600)); // Adjusted size for better layout
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	set_title("Power Prompt");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(30, 30));
	mCloseButton->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
	});
	
	// cached texture
	
	mImageContent = std::make_shared<nanogui::Texture>(
													   nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
													   nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
													   nanogui::Vector2i(2048, 2048),
													   nanogui::Texture::InterpolationMode::Bilinear,
													   nanogui::Texture::InterpolationMode::Nearest,
													   nanogui::Texture::WrapMode::ClampToEdge);
	
	// **New Centering Container for ImageView**
	mImageContainer = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	
	mImageContainer->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 0));
	
	// Image View
	mImageView = std::make_shared<nanogui::ImageView>(*mImageContainer, parent); // Assuming ImageView can take an empty string initially
	mImageView->set_fixed_size(nanogui::Vector2i(300, 300));
	mImageView->set_background_color(nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
	
	mImageView->set_visible(false);
	
	mImageView->set_image(mImageContent);
	
	mPreviewCanvas = std::make_shared<SharedSelfContainedMeshCanvas>(*mImageContainer, parent);
	
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(300, 300));
	
	mPreviewCanvas->set_aspect_ratio(1.0f);

	mPreviewCanvas->set_background_color(nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
	
	mPreviewCanvas->set_visible(true);
	
	// Mesh and Skinned Mesh Batches
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(mRenderPass, mPreviewCanvas->get_mesh_shader());
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(mRenderPass, mPreviewCanvas->get_skinned_mesh_shader());
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Input Panel for Prompt
	mInputPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mInputPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 5, 5));
	
	mInputLabel = std::make_shared<nanogui::Label>(*mInputPanel, "Enter Art Generation Prompt:", "sans-bold");
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
	mDummyAnimationTimeProvider.Update();
	
	mPreviewCanvas->process_events();
	
	if (mActiveActor != nullptr) {
		
		mPreviewCanvas->rotate(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f);
	}
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
	
	std::string prompt_setup = "Analyze this prompt and return a json with boolean fields generate_image, generate_model, generate_rig, generate_animation, prompt_description, animation_description, based on its analysis, only set generate_model to true if the prompt includes the word '3d', if the word '3d' is included and generate_animation is true, then generate_rig must also be true, if the prompt does not include the word '3d' then generate_rig must be false, if the prompt does not include the word 'rig', then generate_rig must be false, if the prompt does not include the word '3d' but generate_animation is true, then generate_image must be true and generate_model and generate_rig must be false, if the prompt includes the word '3d' but does not include the word 'T-pose' or 'A-pose' then generate_animation and generate_rig must be false, parse the prompt, split it and fill prompt_description with the description of the prompt and rig without the animation action, and fill animation_description with the description of the animation prompt if generate_animation is true, if nothing matches, all the fields must be set to false and prompt_description must be an empty string, reply only with the json. Prompt: ";
	
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
		std::string animation_description;
		
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
		
		// Validate and extract 'prompt_description'
		if (root.isMember("animation_description") && root["animation_description"].isString()) {
			animation_description = root["animation_description"].asString();
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
		std::cout << "animation_description: " << animation_description << std::endl;

		if (generate_image && !generate_animation) {
			// Asynchronously generate the image using OpenAiApiClient
			mPowerAi.generate_image_async(prompt_description, [this](std::stringstream image_stream, const std::string& error) {
				if (!error.empty()) {
					std::cerr << "Failed to generate or download image: " << error << std::endl;
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(mStatusMutex);
						mStatusLabel->set_caption("Status: Failed to generate or download image.");
						mGenerateButton->set_enabled(true);
					});
					return;
				}
				
				std::cout << "Image downloaded successfully." << std::endl;
				
				// Convert stringstream to vector<uint8_t>
				std::vector<uint8_t> image_data((std::istreambuf_iterator<char>(image_stream)),
												std::istreambuf_iterator<char>());
				
				if (image_data.empty()) {
					std::cerr << "Downloaded image data is empty." << std::endl;
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(mStatusMutex);
						mStatusLabel->set_caption("Status: Downloaded image data is empty.");
						mGenerateButton->set_enabled(true);
					});
					return;
				}
				
				// Assign image data to mImageData
				mImageData = image_data;
				
				// Update the ImageView with the new texture
				nanogui::async([this]() {
					// Assuming ImageUtils::create_texture_from_data exists and works as expected
					
					nanogui::Texture::decompress_into(mImageData, *mImageContent);
					
					mImageView->set_image(mImageContent);

					mImageContent->resize(nanogui::Vector2i(600, 600));

					mImageView->set_visible(true);
					mPreviewCanvas->set_visible(false);
					
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(mStatusMutex);
						mStatusLabel->set_caption("Status: Model generated successfully.");
						mSaveButton->set_enabled(true);
						
						// Re-enable Generate Button
						mGenerateButton->set_enabled(true);
						
						perform_layout(screen().nvg_context());
						
					});

				});
			});
		} else if (generate_image && generate_animation) {
			std::cerr << "Unsupported prompt." << std::endl;
			{
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Operation not implemented yet.");
			}
		} else if (generate_model) {
			mPowerAi.generate_mesh_async(prompt_description, animation_description, generate_rig, generate_animation, [this](std::stringstream model_stream, const std::string& error){
				
				if (!error.empty()) {
					std::cerr << "Failed to generate or download model: " << error << std::endl;
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(mStatusMutex);
						mStatusLabel->set_caption("Status: Failed to generate or download model.");
						mGenerateButton->set_enabled(true);
					});
					return;
				}

				
				mActorPath = "generated_model.fbx";
				mOutputDirectory = mResourcesPanel.selected_directory_path();
				
				auto actorName = std::filesystem::path(mActorPath).stem().string();
				
				auto actor = std::make_shared<Actor>(mDummyRegistry);
				
				mMeshActorBuilder->build(*actor, mDummyAnimationTimeProvider, model_stream, mActorPath, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());

				mPreviewCanvas->set_active_actor(actor);
				
				mActiveActor = actor;
				
				mImageView->set_visible(false);
				mPreviewCanvas->set_visible(true);

				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Model generated successfully.");
					mSaveButton->set_enabled(true);
					
					// Re-enable Generate Button
					mGenerateButton->set_enabled(true);
					
					perform_layout(screen().nvg_context());

				});
			});
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

