#include "PromptWindow.hpp"

#include "ShaderManager.hpp"

#include "ai/DeepMotionApiClient.hpp"

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
#include <sstream>


PromptWindow::PromptWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent->screen()), mResourcesPanel(resourcesPanel), mDeepMotionApiClient(deepMotionApiClient) {
	
	set_fixed_size(nanogui::Vector2i(400, 512)); // Adjusted height for additional UI elements
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle));
	set_title("Animation Prompt");
	
	// Close Button
	auto close_button = new nanogui::Button(button_panel(), "X");
	close_button->set_fixed_size(nanogui::Vector2i(20, 20));
	close_button->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
		mPreviewCanvas->set_update(false);
	});
	
	// Preview Canvas
	mPreviewCanvas = new SharedSelfContainedMeshCanvas(this);
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	// Mesh and Skinned Mesh Batches
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(renderpass, mPreviewCanvas->get_mesh_shader());
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(renderpass, mPreviewCanvas->get_skinned_mesh_shader());
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Add Text Box for User Input (e.g., Animation Name)
	auto input_panel = new nanogui::Widget(this);
	input_panel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	
	auto label = new nanogui::Label(input_panel, "Description:", "sans-bold");
	mInputTextBox = new nanogui::TextBox(input_panel, "");
	mInputTextBox->set_fixed_size(nanogui::Vector2i(200, 96));
	
	mInputTextBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mInputTextBox->set_placeholder("Enter the animation generation prompt");
	mInputTextBox->set_font_size(14);
	mInputTextBox->set_editable(true);
	
	// Add Submit Button
	mSubmitButton = new nanogui::Button(this, "Submit");
	mSubmitButton->set_callback([this]() {
		nanogui::async([this]() { this->SubmitPrompt();
		});
	});
	mSubmitButton->set_tooltip("Submit the animation import");
	mSubmitButton->set_fixed_width(256);
	
	// Import Button (if still needed)
	// If "Submit" replaces "Import", you can remove or rename this button
	auto importButton = new nanogui::Button(this, "Import");
	importButton->set_callback([this]() {
		nanogui::async([this]() { this->ImportIntoProject(); });
	});
	importButton->set_tooltip("Import the selected asset with the chosen options");
	importButton->set_fixed_width(256);
	
	mMeshActorExporter = std::make_unique<MeshActorExporter>();
}

void PromptWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	mActorPath = path;
	mOutputDirectory = directory;
	
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;
	
	if (!deserializer.load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return;
	}
	
	Preview(path, directory, deserializer);
	deserializer.read_header_raw(mHashId, sizeof(mHashId));
}

void PromptWindow::Preview(const std::string& path, const std::string& directory, CompressedSerialization::Deserializer& deserializer) {
	set_visible(true);
	set_modal(true);
	
	mActorPath = path;
	mOutputDirectory = directory;
	
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	if (!deserializer.load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return;
	}
	
	deserializer.read_header_raw(mHashId, sizeof(mHashId));
	
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	mMeshActorBuilder->build(*actor, path, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	
	if (path.find(".psk") == std::string::npos) {
		// Mesh rigging is still not implemented
		mSubmitButton->set_enabled(false);
	}
	
	mPreviewCanvas->set_active_actor(actor);
	mPreviewCanvas->set_update(true);
	
	// Add Status Label
	mStatusLabel = new nanogui::Label(this, "Status: Idle", "sans-bold");
	mStatusLabel->set_fixed_size(nanogui::Vector2i(300, 20));
}

void PromptWindow::SubmitPrompt() {
	// Determine file type based on extension
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;
	
	if (!deserializer.load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << mActorPath << "\n";
		return;
	}

	std::string prompt = mInputTextBox->value();
	if (prompt.empty()) {
		// Optionally, show an error message to the user
		std::cerr << "Prompt is empty. Please enter a valid prompt." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Prompt is empty.");
		}
		return;
	}
	
	if (!mDeepMotionApiClient.is_authenticated()) {
		// Optionally, prompt the user to authenticate
		std::cerr << "Client is not authenticated. Please authenticate first." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Not authenticated.");
		}
		return;
	}
	
	// Generate hash IDs (for demonstration, we'll generate SHA-256 hashes of prompt and current time)
	
	std::string hash_id0 = std::to_string(mHashId[0]);
	std::string hash_id1 = std::to_string(mHashId[1]);
	
	// Generate Unique Model Name
	std::string unique_model_name = GenerateUniqueModelName(hash_id1, hash_id1);
	std::cout << "Generated Unique Model Name: " << unique_model_name << std::endl;
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Uploading model...");
	}
	

	std::stringstream modelData;

	mMeshActorExporter->exportToStream(deserializer, mActorPath, modelData);
		
	// Upload and Store Model
	std::string modelId = mDeepMotionApiClient.upload_model(modelData, unique_model_name, "fbx");
	if (modelId.empty()) {
		std::cerr << "Failed to upload and store model." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to upload model.");
		}
		return;
	}
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Model uploaded. Processing prompt...");
	}
	
	// Send Prompt to DeepMotion API
	std::string request_id = mDeepMotionApiClient.process_text_to_motion(prompt, unique_model_name);
	if (request_id.empty()) {
		std::cerr << "Failed to process text to motion." << std::endl;
		{
			std::lock_guard<std::mutex> lock(mStatusMutex);
			mStatusLabel->set_caption("Status: Failed to process prompt.");
		}
		return;
	}
	
	std::cout << "Submitted prompt. Request ID: " << request_id << std::endl;
	
	// Update Status
	{
		std::lock_guard<std::mutex> lock(mStatusMutex);
		mStatusLabel->set_caption("Status: Processing animation...");
	}
	
	// Poll Job Status Asynchronously
	std::thread([this, request_id]() {
		while (true) {
			Json::Value status = mDeepMotionApiClient.check_job_status(request_id);
			std::string job_status = status.get("status", "UNKNOWN").asString();
			std::cout << "Job Status: " << job_status << std::endl;
			
			if (job_status == "SUCCESS") {
				// Download results
				Json::Value results = mDeepMotionApiClient.download_job_results(request_id);
				// Process results as needed (e.g., download URLs)
				// For example:
				if (results.isMember("download_url")) {
					std::string download_url = results["download_url"].asString();
					std::cout << "Download URL: " << download_url << std::endl;
					
					// Parse the download URL
					std::string protocol, host, path;
					std::string::size_type protocol_pos = download_url.find("://");
					if (protocol_pos != std::string::npos) {
						protocol = download_url.substr(0, protocol_pos);
						protocol_pos += 3;
					} else {
						std::cerr << "Invalid download URL format: " << download_url << std::endl;
						{
							std::lock_guard<std::mutex> lock(mStatusMutex);
							mStatusLabel->set_caption("Status: Invalid download URL.");
						}
						break;
					}
					
					std::string::size_type host_pos = download_url.find("/", protocol_pos);
					if (host_pos != std::string::npos) {
						host = download_url.substr(protocol_pos, host_pos - protocol_pos);
						path = download_url.substr(host_pos);
					} else {
						host = download_url.substr(protocol_pos);
						path = "/";
					}
					
					// Determine port
					int port = 443; // Default HTTPS
					if (protocol == "http") {
						port = 80;
					}
					
					// Initialize HTTP client for download
					httplib::SSLClient download_client(host.c_str(), port);
					download_client.set_compress(false);
					
					// Perform GET request
					auto res_download = download_client.Get(path.c_str());
					if (res_download && res_download->status == 200) {
						// Assuming the response body contains ZIP data
						std::vector<unsigned char> zip_data(res_download->body.begin(), res_download->body.end());
						
						// Decompress ZIP data (use your existing decompress_zip_data function)
						std::vector<std::stringstream> animation_files = Zip::decompress(zip_data);
						
						// Process the extracted animation files as needed
						
						std::filesystem::path path(mActorPath);
						
						auto actorName = path.stem().string();
						
						for (auto& stream : animation_files) {
							
							auto modelData = mMeshActorImporter->process(stream, actorName, mOutputDirectory);
							
							auto& serializer = modelData->mMesh.mSerializer;
							
							
							// Generate the unique hash identifier from the compressed data
							
							std::stringstream compressedData;
							
							serializer->get_compressed_data(compressedData);

							uint64_t hash_id[] = { 0, 0 };
							
							Md5::generate_md5_from_compressed_data(compressedData, hash_id);
							
							// Write the unique hash identifier to the header
							serializer->write_header_raw(hash_id, sizeof(hash_id));
							
							// Proceed with serialization
							
							// no thumbnails yet as this will be animated, then re-serialized in the import method
							serializer->write_header_uint64(0);
							
							CompressedSerialization::Deserializer deserializer;
							
							if (!deserializer.initialize(compressedData)) {
								
								mStatusLabel->set_caption("Status: Unable to deserialize model.");
								
								nanogui::async([this]() {
									mResourcesPanel.refresh_file_view();
								});
								
								return;
							}

							Preview(mActorPath, mOutputDirectory, deserializer);
							
							break;
						}
						
						{
							std::lock_guard<std::mutex> lock(mStatusMutex);
							mStatusLabel->set_caption("Status: Animations Imported.");
						}
					} else {
						std::cerr << "Failed to download animations. HTTP Status: " << (res_download ? std::to_string(res_download->status) : "No Response") << std::endl;
						{
							std::lock_guard<std::mutex> lock(mStatusMutex);
							mStatusLabel->set_caption("Status: Failed to download animations.");
						}
					}
				}
				break;
			} else if (job_status == "FAILURE") {
				std::cerr << "Job failed." << std::endl;
				{
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Job failed.");
				}
				break;
			} else if (job_status == "PROGRESS") {
				float total = status["details"]["total"].asFloat();
				float current = status["details"]["step"].asFloat();
				if (current > total){
					current = total;
				}
				float percentage = (current * 100.0f) / total;
				{
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: In Progress (" + std::to_string(static_cast<int>(percentage)) + "%)");
				}
			} else {
				{
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: " + job_status);
				}
			}
			
			// Wait before next poll
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}
	}).detach();
}


void PromptWindow::ImportIntoProject() {
	std::string animationName = mInputTextBox->value();
	if (animationName.empty()) {
		// Optionally show an error message to the user
		return;
	}
	
	mPreviewCanvas->take_snapshot([this](std::vector<uint8_t>& pixels) {
		auto& serializer = mCompressedMeshData->mMesh.mSerializer;
		
		std::stringstream compressedData;
		
		serializer->get_compressed_data(compressedData);
		
		CompressedSerialization::Deserializer deserializer;
		
		if (!deserializer.initialize(compressedData)) {
			
			mStatusLabel->set_caption("Status: Unable to deserialize model.");

			nanogui::async([this]() {
				mResourcesPanel.refresh_file_view();
			});
			
			return;
		}
		
		uint64_t hash_id[2] = { 0, 0 };
		
		deserializer.read_header_raw(hash_id, sizeof(hash_id));
		
		serializer->write_header_raw(hash_id, sizeof(hash_id));
		
		// Proceed with serialization
		serializer->write_header_uint64(pixels.size());
		serializer->write_header_raw(pixels.data(), pixels.size()); // Corrected variable name
		
		// Since we're enforcing only animations, ensure only animations are persisted
		mCompressedMeshData->persist_animations(); // Assume a method to persist animations with a name
		
		nanogui::async([this]() {
			mResourcesPanel.refresh_file_view();
		});
		
		mPreviewCanvas->set_active_actor(nullptr);
		mPreviewCanvas->set_update(false);
		
		set_visible(false);
		set_modal(false);
	});
}

void PromptWindow::ProcessEvents() {
	mPreviewCanvas->process_events();
}

std::string PromptWindow::GenerateUniqueModelName(const std::string& hash_id0, const std::string& hash_id1) {
	return hash_id0 + hash_id1;
}

