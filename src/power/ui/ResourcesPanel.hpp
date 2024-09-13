#pragma once

#include "Panel.hpp"

#include "filesystem/DirectoryNode.hpp"

#include <nanogui/nanogui.h>

class IActorVisualManager;
class MeshActorLoader;

class ResourcesPanel : public Panel {
public:
	ResourcesPanel(nanogui::Widget &parent,
				   const DirectoryNode& root_directory_node, IActorVisualManager& actorVisualManager,  MeshActorLoader& meshActorLoader);
	
	void refresh_file_view();
	int get_icon_for_file(const DirectoryNode& node);
	void handle_file_interaction(DirectoryNode& node);
	void refresh();
	
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

private:
	const DirectoryNode& mRootDirectoryNode;
	std::string mSelectedDirectoryPath;
	
	nanogui::Widget *mFileView;
	
	// New UI elements
	nanogui::Widget *mToolbar;
	nanogui::Button *mImportButton;
	nanogui::Button *mExportButton; // New Export button

	nanogui::TextBox *mFilterBox; // New filter input box
	
	// New member variable
	std::string mFilterText;
	

	// New methods
	void import_assets();
	void export_assets();
	void navigate_up_to_cwd();

	IActorVisualManager& mActorVisualManager;
	MeshActorLoader& mMeshActorLoader;
};
