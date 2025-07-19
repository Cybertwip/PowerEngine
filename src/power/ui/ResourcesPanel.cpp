#include "ui/ResourcesPanel.hpp"

#include "ui/FileView.hpp"
#include "ui/ImportWindow.hpp"
#include "ui/UiManager.hpp"

#include <nanogui/nanogui.h>
#include <nanogui/icons.h>
#include <nanogui/opengl.h>

#include <filesystem>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace {
// Helper function to delete a file or directory
bool DeleteFilePath(const std::string& path) {
	try {
		if (!fs::exists(path)) {
			std::cerr << "The specified path does not exist: " << path << std::endl;
			return false;
		}
		fs::remove_all(path);
		std::cout << "Successfully deleted: " << path << std::endl;
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error deleting the file or directory: " << e.what() << std::endl;
		return false;
	}
}

// Helper function to rename a file or directory
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
}


ResourcesPanel::ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen, DirectoryNode& root_directory_node, AnimationTimeProvider& animationTimeProvider, UiManager& uiManager)
: Panel(parent, "Resources"),
mRootDirectoryNode(root_directory_node),
mGlobalAnimationTimeProvider(animationTimeProvider),
mUiManager(uiManager)
{
	// Set the layout
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	// Create the toolbar at the top
	mToolbar = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mToolbar->set_layout(std::make_unique<nanogui::BoxLayout>(
															  nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));
	
	mImportWindow = std::make_shared<ImportWindow>(screen, *this);
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
	mExportButton->set_tooltip("Export Movie");
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
		// Double-click action is now empty as mesh loading is removed
	});
	
	mSelectedDirectoryPath = fs::current_path().string();
	mFilterText = "";
	
	nanogui::async([this](){
		refresh_file_view();
	});
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
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	if (node.FileName.find(".png") != std::string::npos)
		return FA_PHOTO_VIDEO;
	
	// Default icon for other file types
	return FA_FILE;
}

void ResourcesPanel::refresh() {
	nanogui::async([this](){
		refresh_file_view();
	});
}

void ResourcesPanel::import_assets() {
	nanogui::async([this]() {
		nanogui::file_dialog_async(
								   {{"*.*", "All Files"}}, false, false, [this](const std::vector<std::string>& files) {
									   for (const auto& file : files) {
										   try {
											   // Copy the selected file to the current directory
											   std::string destinationPath = DirectoryNode::getInstance().FullPath + "/" + std::filesystem::path(file).filename().string();
											   std::filesystem::copy(file, destinationPath, std::filesystem::copy_options::overwrite_existing);
											   
											   // Show the import window (no 3D preview)
											   mImportWindow->Preview(file, DirectoryNode::getInstance().FullPath);
											   
										   } catch (const std::exception& e) {
											   std::cerr << "Error importing asset: " << e.what() << std::endl;
										   }
									   }
									   
									   // Refresh the file view
									   nanogui::async([this]() {
										   DirectoryNode::getInstance().refresh();
										   refresh_file_view();
									   });
								   });
	});
}

void ResourcesPanel::export_assets() {
	nanogui::async([this]() {
		nanogui::file_dialog_async(
								   {{"mp4", "MP4 Video"}}, true, false, [this](const std::vector<std::string>& files) {
									   if (files.empty()) {
										   return; // User canceled
									   }
									   
									   std::string destinationFile = files.front();
									   mUiManager.export_movie(destinationFile);
								   });
	});
}
void ResourcesPanel::process_events() {
	mImportWindow->ProcessEvents();
	mFileView->ProcessEvents();
}
