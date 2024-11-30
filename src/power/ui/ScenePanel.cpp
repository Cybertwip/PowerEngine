#include "ui/ScenePanel.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>

ScenePanel::ScenePanel(nanogui::Widget& parent, const std::string& title)
: Panel(parent, title)
, mDragging(false)
, mMovable(false) {
	set_position(nanogui::Vector2i(0, 0));
}

bool ScenePanel::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	
	if (Widget::mouse_button_event(p, button, down, modifiers)) {
		return true;
	} 
	// Queue the button up event
	mDragging = down;
	
	mClickQueue.push_back(std::make_tuple(down, width(), height(), p.x(), p.y(), button));

	return true;
}

bool ScenePanel::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	
	if (Widget::mouse_motion_event(p, rel, button, modifiers)) {
		return mMovable;
	} else {
		// Queue the motion event
		mMotionQueue.push_front(std::make_tuple(width(), height(), p.x(), p.y(), rel.x(), rel.y(), button, mDragging));
		
		return false;
	}
}

bool ScenePanel::scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) {
	if (Widget::scroll_event(p, rel)) {
		return true;
	} else {
		// Queue the scroll event
		mScrollQueue.push_front(std::make_tuple(width(), height(), p.x(), p.y(), rel.x(), rel.y()));
		return false;
	}
}

void ScenePanel::register_scroll_callback(std::function<void(int, int, int, int, float, float)> callback) {
	mScrollCallbacks.push_front(callback);
}

void ScenePanel::register_motion_callback(int button, std::function<void(int, int, int, int, int, int, int, bool)> callback) {
	mMotionCallbacks[button].push_back(callback);
}

void ScenePanel::register_click_callback(int button, std::function<void(bool, int, int, int, int)> callback) {
	mClickCallbacks[button].push_back(callback);
}

void ScenePanel::draw(NVGcontext *ctx) {
	// Call the base class draw method
	Panel::draw(ctx);
}

void ScenePanel::process_events() {
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [down, w, h, x, y, button] = mClickQueue.front();
		mClickQueue.pop_front();
		
		// Call callbacks registered for this button
		auto it = mClickCallbacks.find(button);
		if (it != mClickCallbacks.end()) {
			for (auto& callback : it->second) {
				callback(down, w, h, x, y);
			}
		}
	}
	
	// Process motion events
	while (!mMotionQueue.empty()) {
		auto [w, h, x, y, dx, dy, button, down] = mMotionQueue.front();
		mMotionQueue.pop_front();
		
		auto it = mMotionCallbacks.find(button);
		if (it != mMotionCallbacks.end()) {
			for (auto& callback : it->second) {
				callback(w, h, x, y, dx, dy, button, down);
			}
		}
	}
	
	// Process scroll events
	while (!mScrollQueue.empty()) {
		auto [w, h, x, y, dx, dy] = mScrollQueue.front();
		mScrollQueue.pop_front();
		
		for (auto& callback : mScrollCallbacks) {
			callback(w, h, x, y, dx, dy);
		}
	}
}
