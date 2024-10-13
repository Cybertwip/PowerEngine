#include "PromptWindow.hpp"

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

PromptWindow::PromptWindow(nanogui::Screen& parent, nanogui::Screen& screen, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent, screen), mResourcesPanel(resourcesPanel), mDeepMotionApiClient(deepMotionApiClient), mDummyAnimationTimeProvider(60 * 30),
mRenderPass(renderpass) { // update with proper duration, dynamically after loading the animation
	
	set_fixed_size(nanogui::Vector2i(400, 512)); // Adjusted height for additional UI elements
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle));
	set_title("Animation Prompt");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), screen, "X");
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
	mPreviewCanvas = std::make_shared<SharedSelfContainedMeshCanvas>(*this, screen);
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	// Mesh and Skinned Mesh Batches
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(mRenderPass, mPreviewCanvas->get_mesh_shader());
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(mRenderPass, mPreviewCanvas->get_skinned_mesh_shader());
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Add Text Box for User Input (e.g., Animation Name)
	mInputPanel = std::make_shared<nanogui::Widget>(*this, screen);
	mInputPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	
	mInputLabel = std::make_shared<nanogui::Label>(*mInputPanel, screen, "Preview", "sans-bold");
	mInputTextBox = std::make_shared<nanogui::TextBox>(*mInputPanel, screen, "");
	mInputTextBox->set_fixed_size(nanogui::Vector2i(256, 96));
	
	mInputTextBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mInputTextBox->set_placeholder("Enter the animation generation prompt");
	mInputTextBox->set_font_size(14);
	mInputTextBox->set_editable(true);
	
	mImportPanel = std::make_shared<nanogui::Widget>(*this, screen);
	mImportPanel->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 0, 4));
	
	// Add Submit Button
	mSubmitButton = std::make_shared<nanogui::Button>(*mImportPanel, screen, "Submit");
	mSubmitButton->set_callback([this]() {
		nanogui::async([this]() { this->SubmitPromptAsync(); });
	});
	mSubmitButton->set_tooltip("Submit the animation import");
	mSubmitButton->set_fixed_width(208);
	
	mImportButton = std::make_shared<nanogui::Button>(*mImportPanel, screen, "");
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
	mStatusLabel = std::make_shared<nanogui::Label>(*this, screen, "Status: Idle", "sans-bold");
	mStatusLabel->set_fixed_size(nanogui::Vector2i(300, 20));
}

void PromptWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	mActorPath = path;
	mOutputDirectory = directory;
	
	mImportButton->set_enabled(false);
	
	std::filesystem::path filePath(mActorPath);
	std::string extension = filePath.extension().string();
	
	std::string actorName = std::filesystem::path(mActorPath).stem().string();
	
	// Create Deserializer
	CompressedSerialization::Deserializer deserializer;
	
	if (!deserializer.load_from_file(mActorPath)) {
		std::cerr << "Failed to load serialized file: " << path << "\n";
		return;
	}
	
	Preview(path, directory, deserializer, std::nullopt);
}

void PromptWindow::Preview(const std::string& path, const std::string& directory, CompressedSerialization::Deserializer& deserializer, std::optional<std::shared_ptr<PlaybackData>> playbackData) {
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
	} else {
		
		if (playbackData.has_value()) {
			auto& playbackComponent = actor->get_component<PlaybackComponent>();
			
			auto parsedPlayback = std::make_shared<PlaybackData>(playbackComponent.getPlaybackData()->mSkeleton, std::move((*playbackData)->mAnimation));
			
			playbackComponent.setPlaybackData(parsedPlayback);
		}
	}

	
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
			mStatusLabel->set_caption("Status: Uploading model...");
		});
		
		// Asynchronously upload the model
		mDeepMotionApiClient.upload_model_async(std::move(*modelData), unique_model_name, "fbx",
												[this, prompt](const std::string& modelId, const std::string& error) {
			if (!error.empty()) {
				std::cerr << "Failed to upload model: " << error << std::endl;
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Failed to upload model.");
					mSubmitButton->set_enabled(true);
				});
				return;
			}
			
			std::cout << "Model uploaded successfully. Model ID: " << modelId << std::endl;
			nanogui::async([this]() {
				std::lock_guard<std::mutex> lock(mStatusMutex);
				mStatusLabel->set_caption("Status: Model uploaded. Processing prompt...");
			});
			
			// Asynchronously process text to motion
			mDeepMotionApiClient.process_text_to_motion_async(prompt, modelId,
															  [this](const std::string& request_id, const std::string& error) {
				if (!error.empty()) {
					std::cerr << "Failed to process text to motion: " << error << std::endl;
					nanogui::async([this]() {
						std::lock_guard<std::mutex> lock(mStatusMutex);
						mStatusLabel->set_caption("Status: Failed to process prompt.");
						mSubmitButton->set_enabled(true);
					});
					return;
				}
				
				std::cout << "Submitted prompt. Request ID: " << request_id << std::endl;
				nanogui::async([this]() {
					std::lock_guard<std::mutex> lock(mStatusMutex);
					mStatusLabel->set_caption("Status: Processing animation...");
				});
				
				// Start polling job status asynchronously
				PollJobStatusAsync(request_id);
			}
															  );
		}
												);
	}
											);
}

void PromptWindow::PollJobStatusAsync(const std::string& request_id) {
	// Lambda to poll job status
	auto poll = [this, request_id]() {
		bool keep_polling = true;
		while (keep_polling) {
			std::promise<Json::Value> status_promise;
			std::future<Json::Value> status_future = status_promise.get_future();
			
			// Asynchronously check job status
			mDeepMotionApiClient.check_job_status_async(request_id,
														[&status_promise](const Json::Value& status, const std::string& error) {
				if (!error.empty()) {
					std::cerr << "Failed to check job status: " << error << std::endl;
					status_promise.set_value(Json::Value()); // Return empty on error
					return;
				}
				status_promise.set_value(status);
			}
														);
			
			Json::Value status = status_future.get();
			
			if (status.empty()) {
				// Handle error or retry
				std::this_thread::sleep_for(std::chrono::seconds(3));
				continue;
			}
			
			int status_count = status["count"].asInt();
			
			if (status_count > 0) {
				for(auto& json : status["status"]){
					auto job_status = json["status"].asString();
					
					std::cout << "Job Status: " << job_status << std::endl;
					
					if (job_status == "SUCCESS") {
						keep_polling = false;
						// Asynchronously download results
						mDeepMotionApiClient.download_job_results_async(request_id,
																		[this](const Json::Value& results, const std::string& error) {
							if (!error.empty()) {
								std::cerr << "Failed to download animations: " << error << std::endl;
								nanogui::async([this]() {
									std::lock_guard<std::mutex> lock(mStatusMutex);
									mStatusLabel->set_caption("Status: Failed to download animations.");
									mSubmitButton->set_enabled(true); // Re-enable Submit button
								});
								return;
							}
							
							// Process the results
							if (results.isMember("links")) {
								Json::Value urls = results["links"][0]["urls"];
								
								for(auto& url : urls){
									for(auto& file : url["files"]){
										
										if(file.isMember("fbx")){
											auto download_url = file["fbx"].asString();
											
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
													
													mSerializedPrompt = std::move(modelData->mAnimations.value()[0].mSerializer);
													
													std::stringstream animationCompressedData;
													
													mSerializedPrompt.value()->get_compressed_data(animationCompressedData);
													
													CompressedSerialization::Deserializer animationDeserializer;
													
													animationDeserializer.initialize(animationCompressedData);
													
													auto animation = std::make_unique<Animation>();
													
													animation->deserialize(animationDeserializer);
													
													auto playbackData = std::make_shared<PlaybackData>(std::make_shared<Skeleton>(), std::move(animation));
													
													Preview(mActorPath, mOutputDirectory, deserializer, playbackData);
													
													break;
												}
												
												{
													std::lock_guard<std::mutex> lock(mStatusMutex);
													mStatusLabel->set_caption("Status: Animations Imported.");
												}
												
												// Enable the Import button and re-enable Submit button
												nanogui::async([this]() {
													mImportButton->set_enabled(true);
													mSubmitButton->set_enabled(true); // Re-enable Submit button
												});
												
												return;
											} else {
												std::cerr << "Failed to download animations. HTTP Status: " << (res_download ? std::to_string(res_download->status) : "No Response") << std::endl;
												{
													std::lock_guard<std::mutex> lock(mStatusMutex);
													mStatusLabel->set_caption("Status: Failed to download animations.");
												}
											}
											break;
										}
									}
								}
							}
						}
																		);
					} else if (job_status == "FAILURE") {
						keep_polling = false;
						
						std::cerr << "Job failed." << std::endl;
						
						std::string messsage = status["details"]["exc_message"].asString();
						std::string type = status["details"]["exc_type"].asString();
						
						std::cerr << "Failure: " << messsage << std::endl;
						
						std::cerr << "Exception: " << type << std::endl;
						
						
						{
							std::lock_guard<std::mutex> lock(mStatusMutex);
							mStatusLabel->set_caption("Status: Job failed.");
						}
						break;
					} else if (job_status == "PROGRESS") {
						float total = json["details"]["total"].asFloat();
						float current = json["details"]["step"].asFloat();
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
					
					break;
				}
			}
			
			// Wait before next poll
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}
	};
	
	// Launch polling in a separate thread
	std::thread(poll).detach();
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
