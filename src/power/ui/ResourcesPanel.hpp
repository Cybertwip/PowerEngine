#pragma once

#include "Panel.hpp"
#include "animation/AnimationTimeProvider.hpp"

#include <nanogui/widget.h>
#include <memory>
#include <string>

// Forward declarations
class DirectoryNode;
class FileView;
class ImportWindow;
class UiManager;
namespace nanogui {
class Screen;
class Button;
class TextBox;
}


class ResourcesPanel : public Panel {
public:
	// MODIFIED: Constructor now accepts a shared_ptr for the root node to ensure safe memory management.
	ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen,
				   std::shared_ptr<DirectoryNode> root_directory_node,
				   AnimationTimeProvider& animationTimeProvider, UiManager& uiManager);
	
	~ResourcesPanel();
	
	void refresh_file_view();
	int get_icon_for_file(const DirectoryNode& node);
	void refresh();
	
	const std::string& selected_directory_path() const {
		return mSelectedDirectoryPath;
	}
	
	void process_events();
	
private:
	void import_assets();
	void export_assets();
	
	AnimationTimeProvider& mGlobalAnimationTimeProvider;
	// MODIFIED: Use a shared_ptr to manage the root node's lifetime, preventing dangling references.
	std::shared_ptr<DirectoryNode> mRootDirectoryNode;
	std::string mSelectedDirectoryPath;
	
	std::shared_ptr<FileView> mFileView;
	
	// UI elements
	std::shared_ptr<nanogui::Widget> mToolbar;
	std::shared_ptr<nanogui::Button> mImportButton;
	std::shared_ptr<nanogui::Button> mExportButton;
	std::shared_ptr<nanogui::TextBox> mFilterBox;
	
	std::string mFilterText;
	
	// The currently selected node in the FileView.
	std::shared_ptr<DirectoryNode> mSelectedNode;
	
	std::shared_ptr<ImportWindow> mImportWindow;
	
	UiManager& mUiManager;
};
