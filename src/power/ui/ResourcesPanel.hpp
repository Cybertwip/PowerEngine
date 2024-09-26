#pragma once

#include "Panel.hpp"

#include "filesystem/DirectoryNode.hpp"

#include <nanogui/nanogui.h>

#include <entt/entt.hpp>

class IActorVisualManager;
class BatchUnit;
class MeshActorImporter;
class MeshActorLoader;
class MeshActorBuilder;
class SelfContainedMeshCanvas;
class ShaderManager;
class ShaderWrapper;

class ResourcesPanel : public Panel {
public:
	ResourcesPanel(nanogui::Widget &parent,
				   const DirectoryNode& root_directory_node, IActorVisualManager& actorVisualManager,  MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, BatchUnit& batchUnit);
	
	void refresh_file_view();
	int get_icon_for_file(const DirectoryNode& node);
	void handle_file_interaction(DirectoryNode& node);
	void refresh();
	
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

private:
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	
	const DirectoryNode& mRootDirectoryNode;
	std::string mSelectedDirectoryPath;
	
	nanogui::Widget *mFileView;
	
	// New UI elements
	nanogui::Widget *mToolbar;
	nanogui::PopupButton *mAddButton;
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
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	std::unique_ptr<ShaderWrapper> mMeshShader;
	std::unique_ptr<ShaderWrapper> mSkinnedShader;

	std::vector<nanogui::Button*> mFileButtons;
	nanogui::Button* mSelectedButton;
	std::shared_ptr<DirectoryNode> mSelectedNode;
	nanogui::Color mNormalButtonColor;
	nanogui::Color mSelectedButtonColor;
	
	SelfContainedMeshCanvas* mOffscreenRenderer;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	
	entt::registry mDummyRegistry;
};
