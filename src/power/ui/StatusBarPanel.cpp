#include "ui/StatusBarPanel.hpp"
#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

StatusBarPanel::StatusBarPanel(nanogui::Widget &parent) : Panel(parent, "") {
	set_layout(new nanogui::GroupLayout());
	
	// Status bar setup
	nanogui::Widget *statusBar = new nanogui::Widget(this);
	statusBar->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal,
												 nanogui::Alignment::Minimum, 0, 0));
	
	// Button to toggle the resources panel
	nanogui::ToolButton *resourcesButton = new nanogui::ToolButton(statusBar, FA_FOLDER);
	resourcesButton->set_change_callback([this](bool pushed) { toggle_resources_panel(pushed);
	});
	resourcesButton->set_tooltip("Toggle Resources Panel");
	
	// Resources panel setup
	resourcesPanel = new Panel(parent, "Resources");
	resourcesPanel->set_visible(true);
	// Add widgets to resourcesPanel here
	
	// Initial positioning
	resourcesPanel->set_position(nanogui::Vector2i(0, 0));
	
	resourcesPanel->set_fixed_width(parent.fixed_width());
	resourcesPanel->set_fixed_height(parent.fixed_height() * 0.75f);
}

void StatusBarPanel::toggle_resources_panel(bool active) {
	if (animationFuture.valid() && animationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return; // Animation is still running, do not start a new one
	}
	
	if (active) {
		animationFuture = std::async(std::launch::async, [this]() {
			auto target = nanogui::Vector2i(0, parent()->fixed_height() * 0.25f - fixed_height());
			animate_panel_position(target);
		});
	} else {
		animationFuture = std::async(std::launch::async, [this]() {
			auto target = nanogui::Vector2i(0, parent()->fixed_height());
			animate_panel_position(target);
		});
	}
	
	isPanelVisible = active;
	
	resourcesPanel->set_visible(true);
}

void StatusBarPanel::animate_panel_position(const nanogui::Vector2i &targetPosition) {
	const int steps = 10;
	const auto stepDelay = std::chrono::milliseconds(16); // ~60 FPS
	nanogui::Vector2i startPos = resourcesPanel->position();
	nanogui::Vector2i step = (targetPosition - startPos) / (steps - 1);
	
	for (int i = 0; i < steps; ++i) {
		// This check ensures we can stop the animation if another toggle happens
		if (animationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			resourcesPanel->set_position(startPos + step * i);
			std::this_thread::sleep_for(stepDelay);
		} else {
			break; // Stop animation if future is no longer valid (another animation started)
		}
		if (i == steps) {
			resourcesPanel->set_visible(isPanelVisible);
		}
	}
}
