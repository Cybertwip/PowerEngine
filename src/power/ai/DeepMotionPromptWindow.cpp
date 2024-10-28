#include "DeepMotionPromptWindow.hpp"

#include "ShaderManager.hpp"

#include "ai/DeepMotionApiClient.hpp"


#include "components/PlaybackComponent.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/MeshActorExporter.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"
#include "graphics/drawing/SelfContainedSkinnedMeshBatch.hpp"
#include "ui/ResourcesPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/textbox.h>

#include <iostream>
#include <filesystem>
#include <future>
#include <mutex>
#include <sstream>

namespace fs = std::filesystem;

namespace PromptUtils {

// Helper function to generate a unique numeric-based filename
static std::string GenerateUniqueFilename(const std::string& baseDir, const std::string& prefix, const std::string& extension) {
	int maxNumber = 0; // Track the highest number found
	std::regex filenamePattern("^" + prefix + "_(\\d+)\\." + extension + "$"); // Pattern to match filenames with numbers
	
	for (const auto& entry : fs::directory_iterator(baseDir)) {
		if (entry.is_regular_file()) {
			std::smatch match;
			std::string filename = entry.path().filename().string();
			
			if (std::regex_match(filename, match, filenamePattern) && match.size() == 2) {
				int number = std::stoi(match[1].str());
				maxNumber = std::max(maxNumber, number);
			}
		}
	}
	
	// Generate the new filename with the next number
	std::string newFilename = prefix + "_" + std::to_string(maxNumber + 1) + "." + extension;
	return baseDir + "/" + newFilename;
}

}

PromptWindow::PromptWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent), mResourcesPanel(resourcesPanel), mDeepMotionApiClient(deepMotionApiClient), mDummyAnimationTimeProvider(60 * 30),
mRenderPass(renderpass) { // update with proper duration, dynamically after loading the animation
	
	set_fixed_size(nanogui::Vector2i(400, 512)); // Adjusted height for additional UI elements
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle));
	set_title("Animation Prompt");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
		mPreviewCanvas->set_update(false);
		
		// Disable Import Button when closing the window
		nanogui::async([this]() {
			mImportButton->set_enabled(false);
		});
		
	});
	
	// Preview Canvas
	mPreviewCanvas = std::make_shared<SharedSelfContainedMeshCanvas>(*this, parent);
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	// Mesh and Skinned Mesh Batches
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(mRenderPass, mPreviewCanvas->get_mesh_shader());
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(mRenderPass, mPreviewCanvas->get_skinned_mesh_shader());
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Add Text Box for User Input (e.g., Animation Name)
	mInputPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mInputPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	
	mInputLabel = std::make_shared<nanogui::Label>(*mInputPanel, "Preview", "sans-bold");
	mInputTextBox = std::make_shared<nanogui::TextBox>(*mInputPanel, "");
	mInputTextBox->set_fixed_size(nanogui::Vector2i(256, 96));
	
	mInputTextBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mInputTextBox->set_placeholder("Enter the animation generation prompt");
	mInputTextBox->set_font_size(14);
	mInputTextBox->set_editable(true);
	
	mImportPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mImportPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 0, 4));
	
	// Add Submit Button
	mSubmitButton = std::make_shared<nanogui::Button>(*mImportPanel, "Submit");
	mSubmitButton->set_callback([this]() {
		nanogui::async([this]() { this->SubmitPromptAsync(); });
	});
	mSubmitButton->set_tooltip("Submit the animation import");
	mSubmitButton->set_fixed_width(208);
	
	mImportButton = std::make_shared<nanogui::Button>(*mImportPanel, "");
	mImportButton->set_icon(FA_SAVE);
	mImportButton->set_enabled(false);
	mImportButton->set_callback([this]() {
		nanogui::async([this]() { this->ImportIntoProjectAsync();
		});
	});
	mImportButton->set_tooltip("Import the generated animation");
	mImportButton->set_fixed_width(44);
	
	mMeshActorExporter = std::make_unique<MeshActorExporter>();
	
	// Initialize Status Label
	mStatusLabel = std::make_shared<nanogui::Label>(*this, "Status: Idle", "sans-bold");
	mStatusLabel->set_fixed_size(nanogui::Vector2i(300, 20));
}

void PromptWindow::Preview(const std::string& path, const std::string& directory) {
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;

	set_visible(true);
	set_modal(true);
	
	mActorPath = path;
	mOutputDirectory = directory;
	
	mImportButton->set_enabled(false);
	
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	if (!deserializer.load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return;
	}
	
	deserializer.read_header_raw(mHashId, sizeof(mHashId));
	
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	mMeshActorBuilder->build(*actor, mDummyAnimationTimeProvider, path, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	
	if (path.find(".psk") == std::string::npos) {
		// Mesh rigging is still not implemented
		mSubmitButton->set_enabled(false);
	}
	
	mActiveActor = actor;
	
	mPreviewCanvas->set_update(false);
	mPreviewCanvas->set_active_actor(actor);
	mPreviewCanvas->set_update(true);
	

	
	// Update Status Label to Idle
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Idle");
	}
}

void PromptWindow::SubmitPromptAsync() {
	// Disable Submit Button to prevent multiple submissions
	mSubmitButton->set_enabled(false);
	
	// Disable Import Button since a new submission is starting
	mImportButton->set_enabled(false);
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Preparing to upload model...");
	}
	
	// Determine file type based on extension
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	// Create Deserializer
	auto deserializer = std::make_unique<CompressedSerialization::Deserializer>();
	
	if (!deserializer->load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << mActorPath << "\n";
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to load model.");
		}
		mSubmitButton->set_enabled(true);
		return;
	}
	
	std::string prompt = mInputTextBox->value();
	if (prompt.empty()) {
		// Show error message to the user
		std::cerr << "Prompt is empty. Please enter a valid prompt." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Prompt is empty.");
		}
		mSubmitButton->set_enabled(true);
		return;
	}
	
	if (!mDeepMotionApiClient.is_authenticated()) {
		// Prompt the user to authenticate
		std::cerr << "Client is not authenticated. Please authenticate first." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Not authenticated.");
		}
		mSubmitButton->set_enabled(true);
		return;
	}
	
	// Generate Unique Model Name
	std::string unique_model_name = GenerateUniqueModelName(std::to_string(mHashId[0]), std::to_string(mHashId[1]));
	std::cout << "Generated Unique Model Name: " << unique_model_name << std::endl;
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Exporting model...");
	}
	
	// Prepare a stringstream to hold the exported model data
	auto modelData = std::make_shared<std::stringstream>();
	
	// Asynchronously export the model to the stream
	mMeshActorExporter->exportToStreamAsync(std::move(deserializer), mActorPath, *modelData,
											[this, modelData, unique_model_name, prompt](bool success) {
		if (!success) {
			std::cerr << "Failed to export model to stream." << std::endl;
			nanogui::async([this]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Failed to export model.");
				mSubmitButton->set_enabled(true);
			});
			return;
		}
		
		std::cout << "Model exported successfully." << std::endl;
		nanogui::async([this]() {
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Generating animation...");
		});
		
		// Use mDeepMotionApiClient.generate_animation_async
		mDeepMotionApiClient.generate_animation_async(std::move(*modelData), unique_model_name, "fbx", prompt,
													  [this](std::stringstream animated_model_stream, const std::string& error_message) {
			if (!error_message.empty()) {
				std::cerr << "Failed to generate or download animation: " << error_message << std::endl;
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Failed to generate or download animation.");
					mSubmitButton->set_enabled(true);
				});
				return;
			}
			
			std::cout << "Animation generated successfully." << std::endl;
			
			// Process the animated_model_stream
			auto modelData = mMeshActorImporter->process(animated_model_stream, "generated_model", mOutputDirectory);
			
			if (modelData->mAnimations.has_value()) {
				auto serializedPrompt = std::move(modelData->mAnimations.value()[0].mSerializer);
				
				std::stringstream animationCompressedData;
				
				serializedPrompt->get_compressed_data(animationCompressedData);
				
				CompressedSerialization::Deserializer animationDeserializer;
				
				animationDeserializer.initialize(animationCompressedData);
				
				auto animation = std::make_unique<Animation>();
				
				animation->deserialize(animationDeserializer);
				
				auto& playbackComponent = mActiveActor->get_component<PlaybackComponent>();
				
				auto playbackData = std::make_shared<PlaybackData>(std::move(animation));
				
				playbackComponent.setPlaybackData(playbackData);
				
				mSerializedPrompt = std::move(serializedPrompt);
				
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Animation generated successfully.");
					mImportButton->set_enabled(true);
					mSubmitButton->set_enabled(true);
				});
			} else {
				std::cerr << "No animations found in the generated model." << std::endl;
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Failed to process generated animation.");
					mSubmitButton->set_enabled(true);
				});
			}
		});
	});
}


void PromptWindow::ImportIntoProjectAsync() {
	//	std::string animationName = mInputTextBox->value();
	//	if (animationName.empty()) {
	//		// Optionally show an error message to the user
	//		std::cerr << "Animation name is empty." << std::endl;
	//		{
	//			std::lock_guard<std::mutex> lock(mStatusMutex);
	//			mStatusLabel->set_caption("Status: Animation name is empty.");
	//		}
	//		return;
	//	} // maybe a save dialog
	
	mPreviewCanvas->take_snapshot([this](std::vector<uint8_t>& pixels) {
		
		if (mSerializedPrompt.has_value()) {
			std::stringstream compressedData;
			
			mSerializedPrompt.value()->get_compressed_data(compressedData);
			
			uint64_t hash_id[2] = { 0, 0 };
			
			Md5::generate_md5_from_compressed_data(compressedData, hash_id);
			
			// Write the unique hash identifier to the header
			mSerializedPrompt.value()->write_header_raw(hash_id, sizeof(hash_id));
			
			// Proceed with serialization
			mSerializedPrompt.value()->write_header_uint64(pixels.size());
			
			mSerializedPrompt.value()->write_header_raw(pixels.data(), pixels.size());
			
			// Proceed with serialization
			mSerializedPrompt.value()->write_header_uint64(pixels.size());
			mSerializedPrompt.value()->write_header_raw(pixels.data(), pixels.size()); // Corrected variable name
			
			auto actorName = std::filesystem::path(mActorPath).stem().string();
			
			auto animationName = PromptUtils::GenerateUniqueFilename(mOutputDirectory, actorName, "pan");
			
			mSerializedPrompt.value()->save_to_file(animationName);
			
			nanogui::async([this]() {
				mResourcesPanel.refresh_file_view();
			});
			
			mPreviewCanvas->set_active_actor(nullptr);
			mPreviewCanvas->set_update(false);
			
			set_visible(false);
			set_modal(false);
		}
	});
}

void PromptWindow::ProcessEvents() {
	mDummyAnimationTimeProvider.Update();
	
	mPreviewCanvas->process_events();
}

std::string PromptWindow::GenerateUniqueModelName(const std::string& hash_id0, const std::string& hash_id1) {
	return hash_id0 + hash_id1;
}
