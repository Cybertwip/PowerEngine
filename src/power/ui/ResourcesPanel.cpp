#include "ui/ResourcesPanel.hpp"

#include "ai/DeepMotionSettingsWindow.hpp"
#include "ai/PromptWindow.hpp"
#include "actors/IActorSelectedRegistry.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/MeshActorExporter.hpp"
#include "ui/ImportWindow.hpp"
#include "ui/MeshPicker.hpp"
#include "ui/UiManager.hpp"

#include "MeshActorLoader.hpp"
#include "ShaderManager.hpp"

#include <nanogui/nanogui.h>
#include <nanogui/icons.h>
#include <nanogui/opengl.h>

#include <filesystem>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace {
bool DeleteFilePath(const std::string& path) {
	try {
		// Check if the path exists
		if (!fs::exists(path)) {
			std::cerr << "The specified path does not exist: " << path << std::endl;
			return false;
		}
		
		// Remove the file or directory at the given path
		fs::remove_all(path); // use remove_all to also delete directories non-empty
		std::cout << "Successfully deleted: " << path << std::endl;
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error deleting the file or directory: " << e.what() << std::endl;
		return false;
	}
}

// Helper function to generate a unique numeric-based filename
std::string GenerateUniqueFilename(const std::string& baseDir, const std::string& prefix, const std::string& extension) {
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

bool RenamePreview(const std::string& oldPath, const std::string& newName) {
	try {
		fs::path sourcePath(oldPath);
		fs::path targetPath = sourcePath.parent_path() / newName;
		if (fs::exists(targetPath)) {
			std::cerr << "Target file or directory already exists: " << targetPath << std::endl;
			return false;
		}
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error renaming file or directory: " << e.what() << std::endl;
		return false;
	}
}

bool RenameFileOrDirectory(const std::string& oldPath, const std::string& newName) {
	try {
		fs::path sourcePath(oldPath);
		fs::path targetPath = sourcePath.parent_path() / newName;
		if (fs::exists(targetPath)) {
			std::cerr << "Target file or directory already exists: " << targetPath << std::endl;
			return false;
		}
		
		fs::rename(sourcePath, targetPath);
		std::cout << "Renamed " << oldPath << " to " << targetPath << std::endl;
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error renaming file or directory: " << e.what() << std::endl;
		return false;
	}
}

const DirectoryNode* FindNodeByPath(const DirectoryNode& currentNode, const std::string& path) {
	if (currentNode.FullPath == path) {
		return &currentNode;
	}
	for (const auto& child : currentNode.Children) {
		if (const DirectoryNode* found = FindNodeByPath(*child, path)) {
			return found;
		}
	}
	return nullptr; // Not found
}
}


ResourcesPanel::ResourcesPanel(std::shared_ptr<nanogui::Widget> parent, DirectoryNode& root_directory_node, std::shared_ptr<IActorVisualManager> actorVisualManager, std::shared_ptr<SceneTimeBar> sceneTimeBar,  MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient, UiManager& uiManager)
: Panel(parent, "Resources"),
mDummyAnimationTimeProvider(60 * 30),
mRootDirectoryNode(root_directory_node),
mActorVisualManager(actorVisualManager),
mMeshActorLoader(meshActorLoader),
mMeshShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("mesh"))),
mSkinnedShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("skinned_mesh"))),
mSelectedButton(nullptr),
mSelectedNode(nullptr),
mNormalButtonColor(nanogui::Color(0.7f, 0.7f, 0.7f, 1.0f)),
mSelectedButtonColor(nanogui::Color(0.5f, 0.5f, 0.8f, 1.0f)),
mSceneTimeBar(sceneTimeBar),
mUiManager(uiManager)
{
	mNormalButtonColor = theme()->m_button_gradient_bot_unfocused;
	
	mSelectedButtonColor = mNormalButtonColor + nanogui::Color(0.25f, 0.25f, 0.32f, 1.0f);
	
	// Set the layout
	set_layout(std::make_shared<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	// Create the toolbar at the top
	mToolbar = std::make_shared<nanogui::Widget>(shared_from_this());
	mToolbar->set_layout(std::make_shared<nanogui::BoxLayout>(
												nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));
	
	mPromptWindow = std::make_shared<PromptWindow>(parent->window(), std::dynamic_pointer_cast<ResourcesPanel>(shared_from_this()), deepMotionApiClient, shaderManager.render_pass(), shaderManager);
	
	mMeshPicker = std::make_shared<MeshPicker>(parent->window(), mRootDirectoryNode, [this](const std::string& modelPath){
		mMeshPicker->set_visible(false);
		mMeshPicker->set_modal(false);
		
		mPromptWindow->set_visible(true);
		mPromptWindow->set_modal(true);
		mPromptWindow->Preview(modelPath, mSelectedDirectoryPath);
	});
	
	mMeshPicker->set_visible(false);
	mMeshPicker->set_modal(false);
	
	/* Create the DeepMotion Settings Window (initially hidden) */
	mDeepMotionSettings = std::make_shared<DeepMotionSettingsWindow>(parent->window(), deepMotionApiClient, [this](){
		mMeshPicker->set_visible(true);
		mMeshPicker->set_modal(true);
	});
	mDeepMotionSettings->set_visible(false);
	mDeepMotionSettings->set_modal(false);
	
	mImportWindow = std::make_shared<ImportWindow>(parent->window(), shared_from_this(), shaderManager.render_pass(), shaderManager);
	
	mImportWindow->set_visible(false);
	mImportWindow->set_modal(false);
	
	// Add the Add Asset button with a "+" icon
	mAddButton = std::make_shared<nanogui::PopupButton>(mToolbar, "Add");
	mAddButton->set_icon(FA_PLUS);
	mAddButton->set_chevron_icon(0);
	mAddButton->set_tooltip("Add Asset");
	
	mSceneButton = std::make_shared<nanogui::Button>(mAddButton->popup(), "Scene");
	
	mSceneButton->set_icon(FA_HAND_PAPER);
	
	mAnimationButton = std::make_shared<nanogui::Button>(mAddButton->popup(), "Animation");
	
	mAnimationButton->set_icon(FA_RUNNING);
	
	mAnimationButton->set_callback([this](){
		//		if (deepmotion_settings->session_cookie().empty()) {
		mDeepMotionSettings->set_visible(true);
		mDeepMotionSettings->set_modal(true);
		
		mAddButton->set_pushed(false);
		mAddButton->popup()->set_visible(false);
		//		}
	});
	
	mAddButton->popup()->perform_layout(screen()->nvg_context());
	
	mDeepMotionSettings->perform_layout(screen()->nvg_context());
	
	// Add the Import Assets button with a "+" icon
	mImportButton = std::make_shared<nanogui::Button>(mToolbar, "Import");
	mImportButton->set_icon(FA_UPLOAD);
	mImportButton->set_tooltip("Import Assets");
	mImportButton->set_callback([this]() {
		import_assets();
	});
	
	// Add the Export Assets button
	mExportButton = std::make_shared<nanogui::Button>(mToolbar, "Export");
	mExportButton->set_icon(FA_DOWNLOAD);
	mExportButton->set_tooltip("Export Assets");
	mExportButton->set_callback([this]() {
		export_assets();
	});
		
	// Add the Filter input box
	mFilterBox = std::make_shared<nanogui::TextBox>(mToolbar, "");
	mFilterBox->set_placeholder("Filter");
	mFilterBox->set_editable(true);
	mFilterBox->set_fixed_size(nanogui::Vector2i(150, 25));
	mFilterBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mFilterBox->set_callback([this](const std::string &value) {
		mFilterText = value;
		
		nanogui::async([this](){
			refresh_file_view();
		});
		
		return true;
	});
	
	// Create the file view below the toolbar
	mFileView = std::make_shared<nanogui::Widget>(shared_from_this());
	auto gridLayout = std::make_shared<nanogui::AdvancedGridLayout>(
													  /* columns */ {144, 144, 144, 144, 144, 144, 144, 144}, // Initial column widths (can be adjusted)
													  /* rows */ {},                // Start with no predefined rows
													  /* margin */ 8
													  );
	
	// Optionally, set stretch factors for columns and rows
	gridLayout->set_col_stretch(0, 1.0f);
	gridLayout->set_col_stretch(1, 1.0f);
	gridLayout->set_col_stretch(2, 1.0f);
	gridLayout->set_col_stretch(3, 1.0f);
	gridLayout->set_col_stretch(4, 1.0f);
	gridLayout->set_col_stretch(5, 1.0f);
	gridLayout->set_col_stretch(6, 1.0f);
	gridLayout->set_col_stretch(7, 1.0f);

	
	mFileView->set_layout(gridLayout);


	mSelectedDirectoryPath = fs::current_path().string();
	mFilterText = "";
	
	nanogui::async([this](){
		refresh_file_view();
	});
	
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	mMeshActorExporter = std::make_unique<MeshActorExporter>();
}

ResourcesPanel::~ResourcesPanel() {
}

void ResourcesPanel::refresh_file_view() {
	// Clear the buttons vector and reset selection
	mFileButtons.clear();
	mSelectedButton = nullptr;
	mSelectedNode = nullptr;
	
	
	auto gridLayout = std::dynamic_pointer_cast<nanogui::AdvancedGridLayout>(mFileView->layout());
	
	gridLayout->shed_anchor();

	// Refresh the root directory node to get the latest contents
	mRootDirectoryNode.refresh();
	

	// Clear existing items	
	mFileView->shed_children();

	
	if (!mSelectedDirectoryPath.empty()) {
		// Find the currently selected directory node
		const DirectoryNode* selected_node = FindNodeByPath(mRootDirectoryNode, mSelectedDirectoryPath);
		
		if (selected_node) {
			// Define the number of columns in the grid layout
			const int columns = 8; // Adjust as needed for your UI
			int currentIndex = 0;
			
			// Iterate through each child (file or directory) in the selected directory
			for (const auto& child : selected_node->Children) {
				// Apply filter if any
				if (!mFilterText.empty()) {
					std::string filename = child->FileName;
					std::string filter = mFilterText;
					
					// Convert both filename and filter to lowercase for case-insensitive comparison
					std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
					std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
					
					// Skip files that do not match the filter
					if (filename.find(filter) == std::string::npos) {
						continue;
					}
				}
				
				// Create a container widget for the file item
				std::shared_ptr<Widget> itemContainer = std::make_shared<nanogui::Widget>(mFileView);
				itemContainer->set_fixed_size(nanogui::Vector2i(150, 150)); // Set desired size for each item
				
				// Calculate grid position based on current index and number of columns
				int col = currentIndex % columns;
				int row = currentIndex / columns;
				
				// Define the anchor for this item in the grid
				nanogui::AdvancedGridLayout::Anchor anchor(
														   col, row, 1, 1, // Position (col, row) and size (1x1)
														   nanogui::Alignment::Middle, nanogui::Alignment::Middle // Horizontal and Vertical alignment
														   );
				
				// Retrieve the grid layout from mFileView
				auto gridLayout = std::dynamic_pointer_cast<nanogui::AdvancedGridLayout>(mFileView->layout());
				if (gridLayout) {
					// Register the anchor with the grid layout
					gridLayout->set_anchor(itemContainer, anchor);
				}
				
				// Determine the appropriate icon for the file type
				int file_icon = get_icon_for_file(*child);
				
				// Create the icon button within the item container
				auto iconButton = std::make_shared<nanogui::Button>(itemContainer, "", file_icon);
				iconButton->set_fixed_size(nanogui::Vector2i(128, 128)); // Adjust icon size as needed
				iconButton->set_background_color(mNormalButtonColor); // Set default background color
				
				// Handle thumbnail deserialization for specific file types
				
				auto thumbnailPixels = std::make_shared<std::vector<uint8_t>>();

				if (file_icon == FA_WALKING || file_icon == FA_OBJECT_GROUP) {
					// deserialize thumbnail here
					CompressedSerialization::Deserializer deserializer;
					
					deserializer.load_from_file(child->FullPath);
					
					thumbnailPixels->resize(512 * 512 * 4);
					
					uint64_t thumbnail_size = 0;
					
					uint64_t hash_id[] = { 0, 0 };
					
					deserializer.read_header_raw(hash_id, sizeof(hash_id)); // To increase the offset and read the thumbnail size
					
					deserializer.read_header_uint64(thumbnail_size);
					
					if (thumbnail_size != 0) {
						deserializer.read_header_raw(thumbnailPixels->data(), thumbnail_size);
						
						auto imageView = std::make_shared<nanogui::ImageView>(iconButton);
						imageView->set_size(iconButton->fixed_size());
						
						imageView->set_fixed_size(iconButton->fixed_size());
						
						imageView->set_image(std::make_shared<nanogui::Texture>(
																  thumbnailPixels->data(),
																  thumbnailPixels->size(),
																  512, 512,		  nanogui::Texture::InterpolationMode::Nearest,
																  nanogui::Texture::InterpolationMode::Nearest,
																  nanogui::Texture::WrapMode::ClampToEdge));
						
						imageView->image()->resize(nanogui::Vector2i(288, 288));
						
						
						imageView->set_visible(true);
					}
					
				}
				
				
				// Add a label below the icon to display the file name
				auto nameLabel = std::make_shared<nanogui::Label>(itemContainer, child->FileName);
				
				nameLabel->set_fixed_width(128); // Adjust label width as needed
				nameLabel->set_position(nanogui::Vector2i(0, 110)); // Position the label within the container
				
				nameLabel->set_alignment(nanogui::Label::Alignment::Center);
				
				// Add the icon button to the list of file buttons for future reference
				mFileButtons.push_back(iconButton);
				
				// Set the callback for the icon button to handle interactions
				
				iconButton->set_callback([this, iconButton, thumbnailPixels, child]() {
					int file_icon = get_icon_for_file(*child);
					
					if (file_icon  == FA_WALKING || file_icon == FA_PERSON_BOOTH || file_icon == FA_OBJECT_GROUP) {
						
						auto drag_widget = screen()->drag_widget();

						auto content = std::make_shared<nanogui::ImageView>(drag_widget);
						content->set_size(iconButton->fixed_size());
						
						content->set_fixed_size(iconButton->fixed_size());
						
						
						if (file_icon == FA_PERSON_BOOTH) {
							// using a simple image icon for now
							content->set_image(std::make_shared<nanogui::Texture>("internal/ui/animation.png",	 nanogui::Texture::InterpolationMode::Nearest,						nanogui::Texture::InterpolationMode::Nearest,
																	nanogui::Texture::WrapMode::ClampToEdge));
							
							
						} else {
							content->set_image(std::make_shared<nanogui::Texture>(
																	thumbnailPixels->data(),
																	thumbnailPixels->size(),
																	512, 512,		  nanogui::Texture::InterpolationMode::Nearest,
																	nanogui::Texture::InterpolationMode::Nearest,
																	nanogui::Texture::WrapMode::ClampToEdge));
							
							
						}

						content->image()->resize(nanogui::Vector2i(288, 288));

						content->set_visible(true);

						drag_widget->set_size(iconButton->fixed_size());
						
						auto dragStartPosition = iconButton->absolute_position();
						
						drag_widget->set_position(dragStartPosition);
						drag_widget->perform_layout(screen()->nvg_context());
						
						screen()->set_drag_widget(drag_widget, [this, content, drag_widget, child](){
							
							auto path = child->FullPath;
							
							// Remove drag widget
							drag_widget->remove_child(content);
							
							screen()->set_drag_widget(nullptr, nullptr);
							
							std::vector<std::string> path_vector = { path };
							
							screen()->drop_event(shared_from_this(), path_vector);
						});
					}
					// Handle selection
					if (mSelectedButton && mSelectedButton != iconButton) {
						mSelectedButton->set_background_color(mNormalButtonColor);
					}
					mSelectedButton = iconButton;
					iconButton->set_background_color(mSelectedButtonColor);
					
					// Store the selected node for later use
					mSelectedNode = child;
					
					// Handle double-click for action
					// Handle double-click for action
					static std::chrono::time_point<std::chrono::high_resolution_clock> lastClickTime = std::chrono::time_point<std::chrono::high_resolution_clock>::min();
					auto currentClickTime = std::chrono::high_resolution_clock::now();
					
					// Check if lastClickTime is valid before computing duration
					if (lastClickTime != std::chrono::time_point<std::chrono::high_resolution_clock>::min()) {
						auto clickDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentClickTime - lastClickTime).count();
						
						if (clickDuration < 400) { // 400 ms threshold for double-click
							// Double-click detected
							nanogui::async([this]() {
								handle_file_interaction(*mSelectedNode);
							});
							
							// Reset the double-click state
							lastClickTime = std::chrono::time_point<std::chrono::high_resolution_clock>::min();
						} else {
							// Update lastClickTime for the next click
							lastClickTime = currentClickTime;
						}
					} else {
						// First click: update lastClickTime
						lastClickTime = currentClickTime;
					}
				});
							
				
				// Optionally, set up drag-and-drop or other interactions here
				// For example, initiating a drag widget when dragging the iconButton
				
				// Increment the index for the next item
				currentIndex++;
			}
			
			// After adding all items, adjust the grid layout rows based on the number of items
			auto gridLayout = std::dynamic_pointer_cast<nanogui::AdvancedGridLayout>(mFileView->layout());
			if (gridLayout) {
				int totalItems = mFileButtons.size();
				int requiredRows = (totalItems + columns - 1) / columns; // Ceiling division
				
				// Append rows if the current number of rows is less than required
				while (gridLayout->row_count() < requiredRows) {
					gridLayout->append_row(150, 1.0f); // Example row height and stretch factor
				}
				
				// Optionally, remove excess rows if there are fewer items
				while (gridLayout->row_count() > requiredRows) {
					gridLayout->shed_row();
				}
			}
		}
	}
	
	// Perform layout to apply all changes
	perform_layout(screen()->nvg_context());
}

int ResourcesPanel::get_icon_for_file(const DirectoryNode& node) {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos)
		return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

bool ResourcesPanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (down && button == GLFW_MOUSE_BUTTON_1) {
		std::shared_ptr<Widget> widget = find_widget(p);
		bool clickOnButton = false;
		for (auto fileButton : mFileButtons) {
			if (widget == fileButton) {
				clickOnButton = true;
				break;
			}
		}
		if (!clickOnButton) {
			if (!this->contains(p, true)){
				// Clicked outside of buttons
				if (mSelectedButton) {
					mSelectedButton->set_background_color(mNormalButtonColor);
					mSelectedButton = nullptr;
					mSelectedNode = nullptr;
				}
			}
			
		}
	}
	
	return Panel::mouse_button_event(p, button, down, modifiers);
}


void ResourcesPanel::handle_file_interaction(DirectoryNode& node) {
	if (node.IsDirectory) {
		node.refresh();
		mSelectedDirectoryPath = node.FullPath;
	} else {
		if (node.FileName.find(".seq") != std::string::npos || node.FileName.find(".cmp") != std::string::npos) {
			// Logic for opening sequence or composition
		} else if (node.FileName.find(".fbx") != std::string::npos) {
			mActorVisualManager->add_actor(mMeshActorLoader.create_actor(node.FullPath, mDummyAnimationTimeProvider, *mMeshShader, *mSkinnedShader));
		}
		// Handle other file type interactions...
	}
	
	
	// Reset the selection
	if (mSelectedButton) {
		mSelectedButton->set_background_color(mNormalButtonColor);
		mSelectedButton = nullptr;
		mSelectedNode = nullptr;
	}
	
	nanogui::async([this](){
		refresh_file_view();
	});
}

void ResourcesPanel::refresh() {
	// Here you might want to refresh or rebuild the file view if the directory content changes
	nanogui::async([this](){
		refresh_file_view();
	});
	
}

bool ResourcesPanel::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		// Check for CMD (on macOS) or CTRL (on other platforms) + Up Arrow
		bool isCommand = modifiers & (
#ifdef __APPLE__
									  GLFW_MOD_SUPER
#else
									  GLFW_MOD_CONTROL
#endif
									  );
		
		if (isCommand && key == GLFW_KEY_UP) {
			navigate_up_to_cwd();
			return true; // Event handled
		}
	}
	return Panel::keyboard_event(key, scancode, action, modifiers);
}


void ResourcesPanel::import_assets() {
	// Open a file dialog to select files to import
	nanogui::file_dialog_async(
							   { {"fbx", "All Files"} }, false, false, [this](const std::vector<std::string>& files){
								   
								   for (const auto& file : files) {
									   
									   try {
										   // Copy the selected file to the current directory
										   
										   mImportWindow->Preview(file, mSelectedDirectoryPath);
										   
									   } catch (const std::exception& e) {
										   std::cerr << "Error importing asset: " << e.what() << std::endl;
									   }
								   }
								   
								   // Refresh the file view to display the newly imported assets
								   nanogui::async([this](){
									   refresh_file_view();
								   });
								   
							   });
	
}

void ResourcesPanel::export_assets() {
	if (false) {
		// Open a file dialog to select the destination directory
		nanogui::file_dialog_async(
								   { {"fbx", "All Files"} }, true, false, [this](const std::vector<std::string>& files){
									   if (files.empty()) {
										   return; // User canceled
									   }
									   
									   std::string destinationFile = files.front();
									   
									   try {
										   // Copy files or directories
										   CompressedSerialization::Deserializer deserializer;
										   
										   // Load the serialized file
										   if (!deserializer.load_from_file(mSelectedNode->FullPath)) {
											   std::cerr << "Failed to load serialized file: " << mSelectedNode->FullPath << "\n";
											   return;
										   }
										   
										   mMeshActorExporter->exportToFile(deserializer, mSelectedNode->FullPath, destinationFile);
										   
										   std::cout << "Assets exported to: " << destinationFile << std::endl;
									   } catch (const fs::filesystem_error& e) {
										   std::cerr << "Error exporting assets: " << e.what() << std::endl;
									   }
								   });
		
		
	} else {
		nanogui::file_dialog_async(
								   { {"mp4", "All Files"} }, true, false, [this](const std::vector<std::string>& files){
									   if (files.empty()) {
										   return; // User canceled
									   }
									   
									   std::string destinationFile = files.front();
									   
									   mUiManager.export_movie(destinationFile);
								   });
	}
}

void ResourcesPanel::navigate_up_to_cwd() {
	fs::path cwd = fs::current_path();
	fs::path selectedPath = mSelectedDirectoryPath;
	
	if (selectedPath != cwd) {
		fs::path parentPath = fs::path(mSelectedDirectoryPath).parent_path();
		
		// Prevent navigating above the current working directory
		if (parentPath.string().size() >= cwd.string().size()) {
			mSelectedDirectoryPath = parentPath.string();
			nanogui::async([this](){
				refresh_file_view();
			});
		}
	}
}

void ResourcesPanel::process_events() {
	mImportWindow->ProcessEvents();
	mPromptWindow->ProcessEvents();
}
