#include "ui/ResourcesPanel.hpp"

#include "actors/IActorSelectedRegistry.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/MeshActorExporter.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "ui/FileView.hpp"
#include "ui/ImportWindow.hpp"
#include "ui/MeshPicker.hpp"
//#include "ui/SceneTimeBar.hpp"
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


ResourcesPanel::ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen, DirectoryNode& root_directory_node, std::shared_ptr<IActorVisualManager> actorVisualManager,  AnimationTimeProvider& animationTimeProvider, AnimationTimeProvider& previewTimeProvider, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager,  UiManager& uiManager)
: Panel(parent, "Resources"),
mRootDirectoryNode(root_directory_node),
mActorVisualManager(actorVisualManager),
mMeshActorLoader(meshActorLoader),
mMeshShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("mesh"))),
mSkinnedShader(std::make_unique<ShaderWrapper>(shaderManager.get_shader("skinned_mesh"))),
//mSceneTimeBar(sceneTimeBar),
mGlobalAnimationTimeProvider(animationTimeProvider),
mPreviewTimeProvider(previewTimeProvider),
mUiManager(uiManager),
mShaderManager(shaderManager)
{
	// Set the layout
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	// Create the toolbar at the top
	mToolbar = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mToolbar->set_layout(std::make_unique<nanogui::BoxLayout>(
															  nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));

	mImportWindow = std::make_shared<ImportWindow>(screen, *this, mShaderManager.render_pass(), mShaderManager);
	
	mImportWindow->set_visible(false);
	mImportWindow->set_modal(false);
	
	// Add the Import Assets button with a "+" icon
	mImportButton = std::make_shared<nanogui::Button>(*mToolbar, "Import");
	mImportButton->set_icon(FA_DOWNLOAD);
	mImportButton->set_tooltip("Import Assets");
	mImportButton->set_callback([this]() {
		import_assets();
	});
	
	// Add the Export Assets button
	mExportButton = std::make_shared<nanogui::Button>(*mToolbar, "Export");
	mExportButton->set_icon(FA_UPLOAD);
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
	mFileView = std::make_shared<FileView>(*this, mRootDirectoryNode, false, [this](std::shared_ptr<DirectoryNode> node){
		mSelectedNode = node;
	}, [this](std::shared_ptr<DirectoryNode> node){
		mSelectedNode = node;
		
		auto filename = node->FullPath;
		
		if (filename.find(".psk") != std::string::npos || filename.find(".pma") != std::string::npos){
			mActorVisualManager->add_actor(mMeshActorLoader.create_actor(filename, mGlobalAnimationTimeProvider, mPreviewTimeProvider, *mMeshShader, *mSkinnedShader));
			
//			mSceneTimeBar->refresh_actors();
		}
	});
	
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
	nanogui::async([this](){
		mFileView->refresh();
	});
}

int ResourcesPanel::get_icon_for_file(const DirectoryNode& node) {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".psk") != std::string::npos) return FA_WALKING;
	if (node.FileName.find(".pma") != std::string::npos)
		return FA_OBJECT_GROUP;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	if (node.FileName.find(".png") != std::string::npos)
		return FA_PHOTO_VIDEO;
	
	// More conditions for other file types...
	return FA_FILE; // Default icon
}

void ResourcesPanel::refresh() {
	// Here you might want to refresh or rebuild the file view if the directory content changes
	nanogui::async([this](){
		refresh_file_view();
	});
	
}
//
//bool ResourcesPanel::keyboard_event(int key, int scancode, int action, int modifiers) {
//	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
//		// Check for CMD (on macOS) or CTRL (on other platforms) + Up Arrow
//		bool isCommand = modifiers & (
//#ifdef __APPLE__
//									  GLFW_MOD_SUPER
//#else
//									  GLFW_MOD_CONTROL
//#endif
//									  );
//
//		if (isCommand && key == GLFW_KEY_UP) {
//			navigate_up_to_cwd();
//			return true; // Event handled
//		}
//	}
//	return Panel::keyboard_event(key, scancode, action, modifiers);
//}

void ResourcesPanel::import_assets() {
	nanogui::async([this]() {
		nanogui::file_dialog_async(
								   {{"fbx", "Fbx Files"},
								    {"gr2", "Granny2 Files"}}, false, false, [this](const std::vector<std::string>& files) {
									   for (const auto& file : files) {
										   try {
											   // Copy the selected file to the current directory (singleton's path)
											   std::string destinationPath = DirectoryNode::getInstance().FullPath + "/" + std::filesystem::path(file).filename().string();
											   std::filesystem::copy(file, destinationPath, std::filesystem::copy_options::overwrite_existing);
											   
											   // Preview the imported file
											   mImportWindow->Preview(file, DirectoryNode::getInstance().FullPath);
											   
										   } catch (const std::exception& e) {
											   std::cerr << "Error importing asset: " << e.what() << std::endl;
										   }
									   }
									   
									   // Refresh the singleton and file view
									   nanogui::async([this]() {
										   DirectoryNode::getInstance().refresh();
										   refresh_file_view();
									   });
								   });
	});
}

void ResourcesPanel::export_assets() {
	if (mSelectedNode != nullptr) {
		nanogui::async([this]() {
			nanogui::file_dialog_async(
									   {{"fbx", "Fbx Files"}}, true, false, [this](const std::vector<std::string>& files) {
										   if (files.empty()) {
											   return; // User canceled
										   }
										   
										   std::string destinationFile = files.front();
										   
										   try {
											   CompressedSerialization::Deserializer deserializer;
											   
											   // Load the serialized file
											   if (!deserializer.load_from_file(mSelectedNode->FullPath)) {
												   std::cerr << "Failed to load serialized file: " << mSelectedNode->FullPath << "\n";
												   return;
											   }
											   
											   mMeshActorExporter->exportToFile(deserializer, mSelectedNode->FullPath, destinationFile);
											   
											   std::cout << "Assets exported to: " << destinationFile << std::endl;
										   } catch (const std::filesystem::filesystem_error& e) {
											   std::cerr << "Error exporting assets: " << e.what() << std::endl;
										   }
									   });
		});
	} else {
		nanogui::async([this]() {
			nanogui::file_dialog_async(
									   {{"mp4", "All Files"}}, true, false, [this](const std::vector<std::string>& files) {
										   if (files.empty()) {
											   return; // User canceled
										   }
										   
										   std::string destinationFile = files.front();
										   mUiManager.export_movie(destinationFile);
									   });
		});
	}
}
void ResourcesPanel::process_events() {
	mImportWindow->ProcessEvents();
	mFileView->ProcessEvents();
}
