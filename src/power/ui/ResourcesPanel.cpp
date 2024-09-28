#include "ui/ResourcesPanel.hpp"

#include "ai/DeepMotionSettingsWindow.hpp"
#include "actors/IActorSelectedRegistry.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/MeshActorExporter.hpp"
#include "ui/ImportWindow.hpp"
#include "ui/MeshPicker.hpp"

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


ResourcesPanel::ResourcesPanel(nanogui::Widget& parent, DirectoryNode& root_directory_node, IActorVisualManager& actorVisualManager,  MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient)
: Panel(parent, "Resources"),
mRootDirectoryNode(root_directory_node),
mActorVisualManager(actorVisualManager),
mMeshActorLoader(meshActorLoader),
mMeshShader(std::make_unique<ShaderWrapper>(*shaderManager.get_shader("mesh"))),
mSkinnedShader(std::make_unique<ShaderWrapper>(*shaderManager.get_shader("skinned_mesh"))),
mSelectedButton(nullptr),
mSelectedNode(nullptr),
mNormalButtonColor(nanogui::Color(0.7f, 0.7f, 0.7f, 1.0f)),
mSelectedButtonColor(nanogui::Color(0.5f, 0.5f, 0.8f, 1.0f))
{
	mNormalButtonColor = theme()->m_button_gradient_bot_unfocused;
	
	mSelectedButtonColor = mNormalButtonColor + nanogui::Color(0.25f, 0.25f, 0.32f, 1.0f);
	
	// Set the layout
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	// Create the toolbar at the top
	mToolbar = new nanogui::Widget(this);
	mToolbar->set_layout(new nanogui::BoxLayout(
												nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));
	
	mMeshPicker = new MeshPicker(parent.window(), mRootDirectoryNode, [this](const std::string& modelPath){
		
	});
	
	mMeshPicker->set_visible(false);
	mMeshPicker->set_modal(false);

	/* Create the DeepMotion Settings Window (initially hidden) */
	auto deepmotion_settings = new DeepMotionSettingsWindow(parent.window(), deepMotionApiClient, [this](){
		mMeshPicker->set_visible(true);
		mMeshPicker->set_modal(true);
	});
	deepmotion_settings->set_visible(false);
	deepmotion_settings->set_modal(false);
	
	mImportWindow = new ImportWindow(parent.window(), *this, shaderManager.render_pass(), shaderManager);
	
	mImportWindow->set_visible(false);
	mImportWindow->set_modal(false);
	

	
	// Add the Add Asset button with a "+" icon
	mAddButton = new nanogui::PopupButton(mToolbar, "Add");
	mAddButton->set_icon(FA_PLUS);
	mAddButton->set_chevron_icon(0);
	mAddButton->set_tooltip("Add Asset");
	
	auto sceneButton = new nanogui::Button(mAddButton->popup(), "Scene");
	
	sceneButton->set_icon(FA_HAND_PAPER);
	
	auto animationButton = new nanogui::Button(mAddButton->popup(), "Animation");
	
	animationButton->set_icon(FA_RUNNING);
	
	animationButton->set_callback([deepmotion_settings, this](){
		//		if (deepmotion_settings->session_cookie().empty()) {
		deepmotion_settings->set_visible(true);
		deepmotion_settings->set_modal(true);

		mAddButton->set_pushed(false);
		mAddButton->popup()->set_visible(false);
		//		}
	});
	
	mAddButton->popup()->perform_layout(screen()->nvg_context());
	
	deepmotion_settings->perform_layout(screen()->nvg_context());
	
	// Add the Import Assets button with a "+" icon
	mImportButton = new nanogui::Button(mToolbar, "Import");
	mImportButton->set_icon(FA_UPLOAD);
	mImportButton->set_tooltip("Import Assets");
	mImportButton->set_callback([this]() {
		import_assets();
	});
	
	// Add the Export Assets button
	mExportButton = new nanogui::Button(mToolbar, "Export");
	mExportButton->set_icon(FA_DOWNLOAD);
	mExportButton->set_tooltip("Export Assets");
	mExportButton->set_callback([this]() {
		export_assets();
	});
	
	// Add a spacer
	new nanogui::Widget(mToolbar); // Acts as a spacer to push the filter box to the right
	
	// Add the Filter input box
	mFilterBox = new nanogui::TextBox(mToolbar, "");
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
	mFileView = new nanogui::Widget(this);
	mFileView->set_layout(new nanogui::BoxLayout(
												 nanogui::Orientation::Horizontal, nanogui::Alignment::Minimum, 10, 10));
	
	mSelectedDirectoryPath = fs::current_path().string();
	mFilterText = "";
	
	refresh_file_view();

	
	mMeshActorImporter = std::make_unique<MeshActorImporter>();

	mMeshActorExporter = std::make_unique<MeshActorExporter>();
}

ResourcesPanel::~ResourcesPanel() {
}

void ResourcesPanel::refresh_file_view() {
	// Clear the buttons vector
	mFileButtons.clear();
	mSelectedButton = nullptr;

	mRootDirectoryNode.refresh();
	
	// Clear existing items
	auto fileview_children = mFileView->children();
	
	for (auto& file_view_child : fileview_children) {
		auto child_children = file_view_child->children();
		for (auto& child_child : child_children) {
			file_view_child->remove_child(child_child);
		}
		mFileView->remove_child(file_view_child);
	}
	
	
	if (!mSelectedDirectoryPath.empty()) {
		const DirectoryNode* selected_node = FindNodeByPath(mRootDirectoryNode, mSelectedDirectoryPath);
		
		if (selected_node) {
			for (const auto& child : selected_node->Children) {
				// Apply filter
				if (!mFilterText.empty()) {
					std::string filename = child->FileName;
					// Case-insensitive comparison
					std::string filter = mFilterText;
					std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
					std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
					
					if (filename.find(filter) == std::string::npos) {
						continue; // Skip files that don't match the filter
					}
				}
				
				// Create a new widget for each file/directory
				nanogui::Widget *itemContainer = new nanogui::Widget(mFileView);
				itemContainer->set_layout(new nanogui::BoxLayout(
																 nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 5));
				auto file_icon = get_icon_for_file(*child);
				
				auto icon = new nanogui::Button(itemContainer, "", get_icon_for_file(*child));
				
				icon->set_icon(get_icon_for_file(*child));
				icon->set_fixed_size(nanogui::Vector2i(128, 128));
				
				if (file_icon == FA_WALKING) {
					// deserialize thumbnail here
					CompressedSerialization::Deserializer deserializer;
					
					deserializer.load_from_file(child->FullPath);
					
					std::vector<uint8_t> pixels;
					pixels.resize(512 * 512 * 4);
					
					uint64_t thumbnail_size = 0;
					
					deserializer.read_header_uint64(thumbnail_size);
					
					if (thumbnail_size != 0) {
						deserializer.read_header_raw(pixels.data(), thumbnail_size);
						
						auto imageView = new nanogui::ImageView(icon);
						imageView->set_size(icon->fixed_size());
						
						imageView->set_fixed_size(icon->fixed_size());
						
						imageView->set_image(new nanogui::Texture(
																  pixels.data(),
																  pixels.size(),
																  nanogui::Texture::InterpolationMode::Bilinear,
																  nanogui::Texture::InterpolationMode::Nearest,
																  nanogui::Texture::WrapMode::Repeat,
																  512,
																  512));
						
						imageView->image()->resize(nanogui::Vector2i(256, 256));
						
						
						imageView->set_visible(true);
					}
					
				}
				
				
				icon->set_background_color(mNormalButtonColor);
				
				auto name = new nanogui::Label(itemContainer, child->FileName);
				name->set_fixed_width(120);
				
				mFileButtons.push_back(icon);
				
				icon->set_callback([this, icon, child]() {
					int file_icon = get_icon_for_file(*child);
					
					if (file_icon  == FA_WALKING || file_icon == FA_PERSON_BOOTH) {
						
						auto drag_widget = screen()->drag_widget();
						
						auto content = new nanogui::Button(drag_widget, "", file_icon);
						
						content->set_font_size(16);
						content->set_background_color(mNormalButtonColor);
						
						content->set_fixed_size(icon->fixed_size() - 20);
						
						drag_widget->set_size(icon->fixed_size());
						
						auto dragStartPosition = icon->absolute_position();
						
						drag_widget->set_position(dragStartPosition);
						drag_widget->perform_layout(screen()->nvg_context());
						
						screen()->set_drag_widget(drag_widget, [this, content, drag_widget, child](){
							
							auto path = child->FullPath;
							
							// Remove drag widget
							drag_widget->remove_child(content);
							
							screen()->set_drag_widget(nullptr, nullptr);
							
							std::vector<std::string> path_vector = { path };
							
							screen()->drop_event(this, path_vector);
						});
					}
					// Handle selection
					if (mSelectedButton && mSelectedButton != icon) {
						mSelectedButton->set_background_color(mNormalButtonColor);
					}
					mSelectedButton = icon;
					icon->set_background_color(mSelectedButtonColor);
					
					// Store the selected node for later use
					mSelectedNode = child;
					
					// Handle double-click for action
					static std::chrono::time_point<std::chrono::high_resolution_clock> lastClickTime = std::chrono::time_point<std::chrono::high_resolution_clock>::min();
					auto currentClickTime = std::chrono::high_resolution_clock::now();
					auto clickDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentClickTime - lastClickTime).count();
					
					if (clickDuration < 400 && lastClickTime != std::chrono::time_point<std::chrono::high_resolution_clock>::min()) {
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
				});
			}
		}
	}
	
	perform_layout(screen()->nvg_context());
}


int ResourcesPanel::get_icon_for_file(const DirectoryNode& node) {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

bool ResourcesPanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (down && button == GLFW_MOUSE_BUTTON_1) {
		nanogui::Widget* widget = find_widget(p);
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
			mActorVisualManager.add_actor(mMeshActorLoader.create_actor(node.FullPath, *mMeshShader, *mSkinnedShader));
		}
		// Handle other file type interactions...
	}
	
	
	// Reset the selection
	if (mSelectedButton) {
		mSelectedButton->set_background_color(mNormalButtonColor);
		mSelectedButton = nullptr;
		mSelectedNode = nullptr;
	}
	
	refresh_file_view();
}

void ResourcesPanel::refresh() {
	// Here you might want to refresh or rebuild the file view if the directory content changes
	refresh_file_view();
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
	std::vector<std::string> files = nanogui::file_dialog(
														  { {"fbx", "All Files"} }, false, false);
	
	for (const auto& file : files) {
		
		try {
			// Copy the selected file to the current directory
			
			mImportWindow->Preview(file, mSelectedDirectoryPath);
			
		} catch (const std::exception& e) {
			std::cerr << "Error importing asset: " << e.what() << std::endl;
		}
	}
	
	// Refresh the file view to display the newly imported assets
	refresh_file_view();
}

void ResourcesPanel::export_assets() {
	if (mSelectedNode != nullptr) {
		// Open a file dialog to select the destination directory
		std::vector<std::string> files = nanogui::file_dialog(
															  { {"fbx", "All Files"} }, true, false);
		
		if (files.empty()) {
			return; // User canceled
		}
		
		std::string destinationFile = files.front();
		
		try {
			// Copy files or directories
			
			
			auto compressedMeshData = std::move(mMeshActorImporter->process(mSelectedNode->FullPath, "."));

			mMeshActorExporter->exportActor(compressedMeshData->mMesh, destinationFile);

			std::cout << "Assets exported to: " << destinationFile << std::endl;
		} catch (const fs::filesystem_error& e) {
			std::cerr << "Error exporting assets: " << e.what() << std::endl;
		}
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
			refresh_file_view();
		}
	}
}
