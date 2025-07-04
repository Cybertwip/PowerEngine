#pragma once

#include "Panel.hpp"

#include "animation/AnimationTimeProvider.hpp"
#include "filesystem/DirectoryNode.hpp"

#include <nanogui/nanogui.h>

#include <entt/entt.hpp>

class IActorVisualManager;
class FileView;
class ImportWindow;
class MeshActorImporter;
class MeshActorExporter;
class MeshActorLoader;
class MeshActorBuilder;
class MeshPicker;
//class SceneTimeBar;
class SelfContainedMeshCanvas;
class ShaderManager;
class ShaderWrapper;
class UiManager;

class ResourcesPanel : public Panel {
public:
	ResourcesPanel(nanogui::Widget& parent, nanogui::Screen& screen,
				   DirectoryNode& root_directory_node, std::shared_ptr<IActorVisualManager> actorVisualManager,
				   AnimationTimeProvider& animationTimeProvider, AnimationTimeProvider& previewTimeProvider, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, UiManager& uiManager);
	
	~ResourcesPanel();
	
	void refresh_file_view();
	int get_icon_for_file(const DirectoryNode& node);
	void refresh();
	
	const std::string& selected_directory_path() const {
		return mSelectedDirectoryPath;
	}
	
	//virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	void process_events();
	
private:
	AnimationTimeProvider& mGlobalAnimationTimeProvider;
	AnimationTimeProvider& mPreviewTimeProvider;
	DirectoryNode& mRootDirectoryNode;
	std::string mSelectedDirectoryPath;
	
	std::shared_ptr<FileView> mFileView;
	
	// New UI elements
	std::shared_ptr<Widget> mToolbar;
//	std::shared_ptr<nanogui::PopupButton> mAddButton;
	std::shared_ptr<nanogui::Button> mImportButton;
	std::shared_ptr<nanogui::Button> mExportButton; // New Export button
	
	std::shared_ptr<nanogui::TextBox> mFilterBox; // New filter input box
	
	// New member variable
	std::string mFilterText;
	
	// New methods
	void import_assets();
	void export_assets();
	
	std::shared_ptr<IActorVisualManager> mActorVisualManager;
	MeshActorLoader& mMeshActorLoader;
	std::unique_ptr<ShaderWrapper> mMeshShader;
	std::unique_ptr<ShaderWrapper> mSkinnedShader;
	
	std::vector<std::shared_ptr<nanogui::Button>> mFileButtons;
	std::shared_ptr<DirectoryNode> mSelectedNode;
	
	std::shared_ptr<ImportWindow> mImportWindow;

	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
	std::unique_ptr<MeshActorExporter> mMeshActorExporter;
	
//	std::shared_ptr<SceneTimeBar> mSceneTimeBar;
	
	UiManager& mUiManager;
	
	ShaderManager& mShaderManager;
};
