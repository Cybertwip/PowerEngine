
/*
 * =====================================================================================
 *
 * Filename:  ResourcesPanel.cpp
 *
 * Description:  Corrected version of ResourcesPanel.cpp
 *
 * =====================================================================================
 */

#include "ui/ResourcesPanel.hpp"
#include "ui/FileView.hpp"
#include "ui/ImportWindow.hpp"
#include "ui/UiManager.hpp"
#include "filesystem/DirectoryNode.hpp" // Make sure this is included

#include <nanogui/nanogui.h>
#include <nanogui/icons.h>
#include <nanogui/opengl.h>

#include <filesystem>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

// NOTE: The header ResourcesPanel.hpp needs to be updated.
// The constructor signature should be changed to accept a std::shared_ptr.
// The member variable mRootDirectoryNode should be a std::shared_ptr<DirectoryNode>.

ResourcesPanel::ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen, std::shared_ptr<DirectoryNode> root_directory_node, AnimationTimeProvider& animationTimeProvider, UiManager& uiManager)
: Panel(parent, "Resources"),
mRootDirectoryNode(root_directory_node), // MODIFIED: Use shared_ptr
mGlobalAnimationTimeProvider(animationTimeProvider),
mUiManager(uiManager)
{
	set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 0, 10));
	
	mToolbar = std::make_shared<nanogui::Widget>(*this);
	mToolbar->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 10, 10));
	
	mImportWindow = std::make_shared<ImportWindow>(screen, *this);
	mImportWindow->set_visible(false);
	mImportWindow->set_modal(false);
	
	mImportButton = std::make_shared<nanogui::Button>(*mToolbar, "Import");
	mImportButton->set_icon(FA_DOWNLOAD);
	mImportButton->set_tooltip("Import Assets");
	mImportButton->set_callback([this]() {
		import_assets();
	});
	
	mExportButton = std::make_shared<nanogui::Button>(*mToolbar, "Export");
	mExportButton->set_icon(FA_UPLOAD);
	mExportButton->set_tooltip("Export Movie");
	mExportButton->set_callback([this]() {
		export_assets();
	});
	
	mFilterBox = std::make_shared<nanogui::TextBox>(*mToolbar, "");
	mFilterBox->set_placeholder("Filter");
	mFilterBox->set_editable(true);
	mFilterBox->set_fixed_size(nanogui::Vector2i(150, 25));
	mFilterBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mFilterBox->set_callback([this](const std::string &value) {
		if (mFileView) {
			mFileView->set_filter_text(value);
		}
		return true;
	});
	
	// Create the file view with the shared_ptr to the root node
	mFileView = std::make_shared<FileView>(*this, mRootDirectoryNode, false,
										   [this](std::shared_ptr<DirectoryNode> node){
		mSelectedNode = node;
	},
										   [this](std::shared_ptr<DirectoryNode> node){
		mSelectedNode = node;
		// Handle double-click if needed
	}
										   );
	
	mFilterText = "";
	refresh_file_view();
}

ResourcesPanel::~ResourcesPanel() {
}

void ResourcesPanel::refresh_file_view() {
	if (mFileView) {
		mFileView->refresh();
	}
}

int ResourcesPanel::get_icon_for_file(const DirectoryNode& node) {
	if (node.IsDirectory) return FA_FOLDER;
	if (node.FileName.find(".pan") != std::string::npos) return FA_PERSON_BOOTH;
	if (node.FileName.find(".png") != std::string::npos) return FA_PHOTO_VIDEO;
	return FA_FILE;
}

void ResourcesPanel::refresh() {
	refresh_file_view();
}

void ResourcesPanel::import_assets() {
	nanogui::file_dialog_async({{"*.fbx", "Model Files"}, {"*.png", "Texture Files"}}, true, false, [this](const std::vector<std::string>& files) {
		for (const auto& file : files) {
			try {
				// MODIFIED: Use the mRootDirectoryNode member instead of getInstance()
				if (mRootDirectoryNode && !mRootDirectoryNode->FullPath.empty()) {
					fs::path destinationPath = fs::path(mRootDirectoryNode->FullPath) / fs::path(file).filename();
					fs::copy(file, destinationPath, fs::copy_options::overwrite_existing);
					mImportWindow->Preview(file, mRootDirectoryNode->FullPath);
				}
			} catch (const std::exception& e) {
				std::cerr << "Error importing asset: " << e.what() << std::endl;
			}
		}
		
		// Refresh the file view after import
		if (mRootDirectoryNode) {
			mRootDirectoryNode->refresh();
		}
		refresh_file_view();
	});
}

void ResourcesPanel::export_assets() {
	nanogui::file_dialog_async({{"mp4", "MP4 Video"}}, true, false, [this](const std::vector<std::string>& files) {
		if (files.empty()) {
			return; // User canceled
		}
		std::string destinationFile = files.front();
		mUiManager.export_movie(destinationFile);
	});
}

void ResourcesPanel::process_events() {
	if(mImportWindow) mImportWindow->ProcessEvents();
	if(mFileView) mFileView->ProcessEvents();
}
