#include "ui/ScenePanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget &parent)
: Panel(parent, "Scene")
, mDragging(false) {
    set_position(nanogui::Vector2i(0, 0));
}

bool ScenePanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	
	if (Widget::mouse_button_event(p, button, down, modifiers)) {
		
		
		// Queue the button up event
		if (button == GLFW_MOUSE_BUTTON_1 && !down) {
			mDragging = down;
			mClickQueue.push(std::make_tuple(down, width(), height(), p.x(), p.y()));
		}

		return true;
	}
	
	if (button == GLFW_MOUSE_BUTTON_1 && down && !m_focused) {
		request_focus();
	}
	
	// Queue the click event
	if (button == GLFW_MOUSE_BUTTON_1) {
		mDragging = down;
		mClickQueue.push(std::make_tuple(down, width(), height(), p.x(), p.y()));
	}
	
	return false;
}

bool ScenePanel::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	
	if (Widget::mouse_motion_event(p, rel, button, modifiers)) {
		return true;
	}

	// Queue the motion event
	mMotionQueue.push(std::make_tuple(width(), height(), p.x(), p.y(), rel.x(), rel.y(), button, mDragging));
		
	return Widget::mouse_motion_event(p, rel, button, modifiers);
}

void ScenePanel::register_motion_callback(std::function<void(int, int, int, int, int, int, int, bool)> callback) {
	mMotionCallbacks.push_back(callback);
}

void ScenePanel::register_click_callback(std::function<void(bool, int, int, int, int)> callback) {
	mClickCallbacks.push_back(callback);
}

void ScenePanel::draw(NVGcontext *ctx) {
	// Call the base class draw method
	Panel::draw(ctx);
}

void ScenePanel::process_events() {
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [down, w, h, x, y] = mClickQueue.front();
		mClickQueue.pop();
		
		for (auto& callback : mClickCallbacks) {
			callback(down, w, h, x, y);
		}
	}
	
	// Process motion events
	while (!mMotionQueue.empty()) {
		auto [w, h, x, y, dx, dy, button, down] = mMotionQueue.front();
		mMotionQueue.pop();
		
		for (auto& callback : mMotionCallbacks) {
			callback(w, h, x, y, dx, dy, button, down);
		}
	}
}

