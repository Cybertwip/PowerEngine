#pragma once

#include "Panel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>

#include <functional>
#include <future>
#include <chrono>

class IActorVisualManager;
class DeepMotionApiClient;
class MeshActorLoader;
class ResourcesPanel;
class SceneTimeBar;
class ShaderManager;
class UiManager;

class StatusBarPanel : public Panel {
public:
	StatusBarPanel(std::shared_ptr<nanogui::Widget> parent, std::shared_ptr<IActorVisualManager> actorVisualManager, std::shared_ptr<SceneTimeBar> sceneTimeBar, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient, UiManager& uiManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	
	std::shared_ptr<ResourcesPanel> resources_panel() {
		return mResourcesPanel;
	}
	
private:
	void initialize() override;
	
	std::shared_ptr<ResourcesPanel> mResourcesPanel;
	bool mIsPanelVisible = false;
	std::future<void> mAnimationFuture;

	void toggle_resources_panel(bool active);
	void animate_panel_position(const nanogui::Vector2i &targetPosition);
	
	std::shared_ptr<SceneTimeBar> mSceneTimeBar;
	
	std::shared_ptr<nanogui::Widget> mStatusBar;
	
	std::shared_ptr<nanogui::ToolButton> mResourcesButton;
	
	std::shared_ptr<IActorVisualManager> mActorVisualManager;
	MeshActorLoader& mMeshActorLoader;
	ShaderManager& mShaderManager;
	DeepMotionApiClient& mDeepMotionApiClient;
	UiManager& mUiManager;
	
	std::function<void(std::function<void(int, int)>)> mApplicationClickRegistrator;
};
