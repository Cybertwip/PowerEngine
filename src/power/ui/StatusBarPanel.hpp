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
class BatchUnit;
class MeshActorLoader;
class ResourcesPanel;
class ShaderManager;

class StatusBarPanel : public Panel {
public:
	StatusBarPanel(nanogui::Widget &parent, IActorVisualManager& actorVisualManager, MeshActorLoader& meshActorLoader, ShaderManager& shaderManager, BatchUnit& batchUnit, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator);
	
	ResourcesPanel& resources_panel() {
		return *mResourcesPanel;
	}
	
private:
	ResourcesPanel *mResourcesPanel;
	bool mIsPanelVisible = false;
	std::future<void> mAnimationFuture;

	void toggle_resources_panel(bool active);
	void animate_panel_position(const nanogui::Vector2i &targetPosition);
};
