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


ResourcesPanel::ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen, DirectoryNode& root_directory_node, std::shared_ptr<IActorVisualManager> actorVisualManager, std::shared_ptr<SceneTimeBar> sceneTimeBar,  MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient, UiManager& uiManager)
: Panel(parent, screen, "Resources"),
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
mUiManager(uiManager),
mDeepMotionApiClient(deepMotionApiClient),
mShaderManager(shaderManager)
{
	
	mNormalButtonColor = theme().m_button_gradient_bot_unfocused;
	
	mSelectedButtonColor = mNormalButtonColor + nanogui::Color(0.25f, 0.25f, 0.32f, 1.0f);
	
	// Set the layout
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	// Create the toolbar at the top
	mToolbar = std::make_shared<nanogui::Widget>(*this, screen);
	mToolbar->set_layout(std::make_unique<nanogui::BoxLayout>(
															  nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));
	
	mPromptWindow = std::make_shared<PromptWindow>(screen, screen, *this, mDeepMotionApiClient, mShaderManager.render_pass(), mShaderManager);
	
	mMeshPicker = std::make_shared<MeshPicker>(screen, screen, mRootDirectoryNode, [this](const std::string& modelPath){
		mMeshPicker->set_visible(false);
		mMeshPicker->set_modal(false);
		
		mPromptWindow->set_visible(true);
		mPromptWindow->set_modal(true);
		mPromptWindow->Preview(modelPath, mSelectedDirectoryPath);
	});
	
	mMeshPicker->set_visible(false);
	mMeshPicker->set_modal(false);
	
	/* Create the DeepMotion Settings Window (initially hidden) */
	mDeepMotionSettings = std::make_shared<DeepMotionSettingsWindow>(screen, mDeepMotionApiClient, [this](){
		mMeshPicker->set_visible(true);
		mMeshPicker->set_modal(true);
	});
	mDeepMotionSettings->set_visible(false);
	mDeepMotionSettings->set_modal(false);
	
	mImportWindow = std::make_shared<ImportWindow>(screen, screen, *this, mShaderManager.render_pass(), mShaderManager);
	
	mImportWindow->set_visible(false);
	mImportWindow->set_modal(false);
	
	// Add the Add Asset button with a "+" icon
	mAddButton = std::make_shared<nanogui::PopupButton>(*mToolbar, screen, "Add");
	mAddButton->set_icon(FA_PLUS);
	mAddButton->set_chevron_icon(0);
	mAddButton->set_tooltip("Add Asset");
	
	mSceneButton = std::make_shared<nanogui::Button>(mAddButton->popup(), "Scene");
	
	mSceneButton->set_icon(FA_HAND_PAPER);
	
	mAnimationButton = std::make_shared<nanogui::Button>(mAddButton->popup(), screen, "Animation");
	
	mAnimationButton->set_icon(FA_RUNNING);
	
	mAnimationButton->set_callback([this](){
		//		if (deepmotion_settings->session_cookie().empty()) {
		mDeepMotionSettings->set_visible(true);
		mDeepMotionSettings->set_modal(true);
		
		mAddButton->set_pushed(false);
		mAddButton->popup().set_visible(false);
		//		}
	});
	
	mAddButton->popup().perform_layout(screen.nvg_context());
	
	mDeepMotionSettings->perform_layout(screen.nvg_context());
	
	// Add the Import Assets button with a "+" icon
	mImportButton = std::make_shared<nanogui::Button>(*mToolbar, screen, "Import");
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
	mFilterBox = std::make_shared<nanogui::TextBox>(*mToolbar, "");
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
	mFileView = std::make_shared<nanogui::Widget>(*this, screen);
	auto gridLayout = std::unique_ptr<nanogui::AdvancedGridLayout>(new nanogui::AdvancedGridLayout(
																								   /* columns */ {144, 144, 144, 144, 144, 144, 144, 144}, // Initial column widths (can be adjusted)
																								   /* rows */ {},                // Start with no predefined rows
																								   /* margin */ 8
																								   ));
	
	// Optionally, set stretch factors for columns and rows
	gridLayout->set_col_stretch(0, 1.0f);
	gridLayout->set_col_stretch(1, 1.0f);
	gridLayout->set_col_stretch(2, 1.0f);
	gridLayout->set_col_stretch(3, 1.0f);
	gridLayout->set_col_stretch(4, 1.0f);
	gridLayout->set_col_stretch(5, 1.0f);
	gridLayout->set_col_stretch(6, 1.0f);
	gridLayout->set_col_stretch(7, 1.0f);
	
	
	mFileView->set_layout(std::move(gridLayout));
	
	
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
		Widget* widget = find_widget(p);
		bool clickOnButton = false;
		for (auto fileButton : mFileButtons) {
			if (widget == fileButton.get()) {
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
