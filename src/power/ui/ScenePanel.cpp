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
	if (button == GLFW_MOUSE_BUTTON_1 && down && !m_focused) {
		request_focus();
	}
	
	
	// Queue the click event
	if (button == GLFW_MOUSE_BUTTON_1) {
		mDragging = down;
		mDragPosition = p;
		mClickQueue.push(std::make_tuple(down, width(), height(), p.x(), p.y()));
	}

	return Widget::mouse_button_event(p, button, down, modifiers);
}

bool ScenePanel::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	// Queue the motion event
	mMotionQueue.push(std::make_tuple(width(), height(), mDragPosition.x(), mDragPosition.y(), mDragPosition.x() - p.x(), mDragPosition.y() - p.y(), button, mDragging));
	
	mDragPosition = p;
	
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

