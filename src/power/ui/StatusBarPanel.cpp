#include "ui/StatusBarPanel.hpp"
#include "ui/ResourcesPanel.hpp"

#include "filesystem/DirectoryNode.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

#include <filesystem>


static std::unique_ptr<DirectoryNode> rootNode = DirectoryNode::create(std::filesystem::current_path().string());

StatusBarPanel::StatusBarPanel(nanogui::Widget &parent, IActorVisualManager& actorVisualManager,  MeshActorLoader& meshActorLoader, std::function<void(std::function<void(int, int)>)> applicationClickRegistrator) : Panel(parent, "") {
	set_layout(new nanogui::GroupLayout());
	
	// Status bar setup
	nanogui::Widget *statusBar = new nanogui::Widget(this);
	statusBar->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
												 nanogui::Alignment::Minimum, 0, 0));
	
	// Button to toggle the resources panel
	nanogui::ToolButton *resourcesButton = new nanogui::ToolButton(statusBar, FA_FOLDER);

	resourcesButton->set_tooltip("Toggle Resources Panel");
	
	resourcesButton->set_enabled(false);
	
	// Resources panel setup
	mResourcesPanel = new ResourcesPanel(*parent.parent(), *rootNode, actorVisualManager, meshActorLoader);
	mResourcesPanel->set_visible(true);
	// Add widgets to resourcesPanel here
	
	// Initial positioning
	mResourcesPanel->set_position(nanogui::Vector2i(0, 0));
	
	mResourcesPanel->set_fixed_width(parent.parent()->fixed_width());
	mResourcesPanel->set_fixed_height(parent.parent()->fixed_height() * 0.75f);
	
	applicationClickRegistrator([this, resourcesButton](int x, int y){
		if (!mResourcesPanel->contains(nanogui::Vector2i(x, y))){
			if (mIsPanelVisible) {
				toggle_resources_panel(false);
				
				resourcesButton->set_pushed(false);
			}
		}
		
		if (resourcesButton->contains(nanogui::Vector2i(x, y), true)) {
			toggle_resources_panel(!mIsPanelVisible);
			resourcesButton->set_pushed(mIsPanelVisible);
		}
	});
}

void StatusBarPanel::toggle_resources_panel(bool active) {
	if (mAnimationFuture.valid() && mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return; // Animation is still running, do not start a new one
	}
	
	if (active) {
		mAnimationFuture = std::async(std::launch::async, [this]() {
			auto target = nanogui::Vector2i(0, parent()->parent()->fixed_height() * 0.25f - fixed_height());
			animate_panel_position(target);
		});
	} else {
		mAnimationFuture = std::async(std::launch::async, [this]() {
			auto target = nanogui::Vector2i(0, parent()->parent()->fixed_height());
			animate_panel_position(target);
		});
	}
	
	mIsPanelVisible = active;
	
	mResourcesPanel->set_visible(true);
}

void StatusBarPanel::animate_panel_position(const nanogui::Vector2i &targetPosition) {
	const int steps = 20;
	const auto stepDelay = std::chrono::milliseconds(16); // ~60 FPS
	nanogui::Vector2f startPos = mResourcesPanel->position();
	nanogui::Vector2f step = (nanogui::Vector2f(targetPosition) - startPos) / (steps - 1);
	
	for (int i = 0; i < steps; ++i) {
		// This check ensures we can stop the animation if another toggle happens
		if (mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			mResourcesPanel->set_position(startPos + step * i);
			std::this_thread::sleep_for(stepDelay);
		} else {
			break; // Stop animation if future is no longer valid (another animation started)
		}
		if (i == steps) {
			mResourcesPanel->set_visible(mIsPanelVisible);
		}
	}
}
