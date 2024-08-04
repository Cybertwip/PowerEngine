#include "ui/ScenePanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget &parent) : Panel(parent, "Scene") {
    set_position(nanogui::Vector2i(0, 0));
    set_layout(
        new nanogui::GridLayout(nanogui::Orientation::Vertical, 1, nanogui::Alignment::Fill));
}

bool ScenePanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (button == GLFW_MOUSE_BUTTON_1 && down && !m_focused) {
		request_focus();
	}
	
	
	// Queue the click event
	
	if (button == GLFW_MOUSE_BUTTON_1 && down) {
		mClickQueue.push(std::make_tuple(width(), height(), p.x(), p.y()));
	}

	return true;
}

void ScenePanel::register_click_callback(std::function<void(int, int, int, int)> callback) {
	mClickCallbacks.push_back(callback);
}

void ScenePanel::draw(NVGcontext *ctx) {
	// Call the base class draw method
	Panel::draw(ctx);
}

void ScenePanel::process_events() {
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [w, h, x, y] = mClickQueue.front();
		mClickQueue.pop();
		
		for (auto& callback : mClickCallbacks) {
			callback(w, h, x, y);
		}
	}
}

