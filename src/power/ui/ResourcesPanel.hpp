#pragma once

#include "Panel.hpp"

#include "animation/AnimationTimeProvider.hpp"
#include "filesystem/DirectoryNode.hpp"

#include <nanogui/nanogui.h>

#include <entt/entt.hpp>

// Forward declarations
class FileView;
class ImportWindow;
class UiManager;

class ResourcesPanel : public Panel {
public:
	ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen,
				   DirectoryNode& root_directory_node,
				   AnimationTimeProvider& animationTimeProvider, AnimationTimeProvider& previewTimeProvider, UiManager& uiManager);
	
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
	AnimationTimeProvider& mPreviewTimeProvider;
	DirectoryNode& mRootDirectoryNode;
	std::string mSelectedDirectoryPath;
	
	std::shared_ptr<FileView> mFileView;
	
	// UI elements
	std::shared_ptr<Widget> mToolbar;
	std::shared_ptr<nanogui::Button> mImportButton;
	std::shared_ptr<nanogui::Button> mExportButton;
	std::shared_ptr<nanogui::TextBox> mFilterBox;
	
	std::string mFilterText;
	
	std::vector<std::shared_ptr<nanogui::Button>> mFileButtons;
	std::shared_ptr<DirectoryNode> mSelectedNode;
	
	std::shared_ptr<ImportWindow> mImportWindow;
	
	UiManager& mUiManager;
};
