#pragma once

#include "Panel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>

#include <future>
#include <chrono>


class StatusBarPanel : public Panel {
public:
	StatusBarPanel(nanogui::Widget &parent);
	
private:
	nanogui::Widget *resourcesPanel;
	bool isPanelVisible = false;
	std::future<void> animationFuture;

	void toggle_resources_panel(bool active);
	void animate_panel_position(const nanogui::Vector2i &targetPosition);
};
