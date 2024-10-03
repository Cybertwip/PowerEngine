#pragma once

#include "Panel.hpp"

#include "animation/AnimationTimeProvider.hpp"
#include "filesystem/DirectoryNode.hpp"

#include <nanogui/nanogui.h>

#include <entt/entt.hpp>

class IActorVisualManager;
class DeepMotionApiClient;
class ImportWindow;
class MeshActorImporter;
class MeshActorExporter;
class MeshActorLoader;
class MeshActorBuilder;
class MeshPicker;
class PromptWindow;
class SceneTimeBar;
class SelfContainedMeshCanvas;
class ShaderManager;
class ShaderWrapper;

class ResourcesPanel : public Panel {
public:
	ResourcesPanel(nanogui::Widget &parent,
				   DirectoryNode& root_directory_node, IActorVisualManager& actorVisualManager,
					   SceneTimeBar& sceneTimeBar, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient);
	
	~ResourcesPanel();
	
	void refresh_file_view();
	int get_icon_for_file(const DirectoryNode& node);
	void handle_file_interaction(DirectoryNode& node);
	void refresh();
	
	virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	void process_events();
	
private:
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	
	AnimationTimeProvider mDummyAnimationTimeProvider;
	
	DirectoryNode& mRootDirectoryNode;
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
	std::unique_ptr<ShaderWrapper> mMeshShader;
	std::unique_ptr<ShaderWrapper> mSkinnedShader;
	
	std::vector<nanogui::Button*> mFileButtons;
	nanogui::Button* mSelectedButton;
	std::shared_ptr<DirectoryNode> mSelectedNode;
	nanogui::Color mNormalButtonColor;
	nanogui::Color mSelectedButtonColor;
	
	ImportWindow* mImportWindow;
	MeshPicker* mMeshPicker;
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	std::unique_ptr<MeshActorExporter> mMeshActorExporter;
	
	PromptWindow* mPromptWindow;
	SceneTimeBar& mSceneTimeBar;
};
