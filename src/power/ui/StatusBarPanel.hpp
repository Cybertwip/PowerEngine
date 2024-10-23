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
class AnimationTimeProvider;
class DeepMotionApiClient;
class MeshActorLoader;
class PowerAi;
class ResourcesPanel;
class SceneTimeBar;
class ShaderManager;
class UiManager;

class StatusBarPanel : public Panel {
public:
	StatusBarPanel(nanogui::Widget& parent, nanogui::Screen& screen, std::shared_ptr<IActorVisualManager> actorVisualManager, std::shared_ptr<SceneTimeBar> sceneTimeBar, AnimationTimeProvider& animationTimeProvider, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, DeepMotionApiClient& deepMotionApiClient, PowerAi& powerAi, UiManager& uiManager, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	
	std::shared_ptr<ResourcesPanel> resources_panel() {
		return mResourcesPanel;
	}
	
private:
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
	UiManager& mUiManager;
	
	std::function<void(std::function<void(int, int)>)> mApplicationClickRegistrator;
};
