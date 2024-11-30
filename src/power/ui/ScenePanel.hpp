#pragma once

#include "Panel.hpp"

#include <deque>
#include <functional>
#include <map>

class ScenePanel : public Panel {
public:
	ScenePanel(nanogui::Widget& parent, const std::string& title = "Scene");
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	void register_click_callback(int button, std::function<void(bool, int, int, int, int)> callback);
	virtual void process_events();
	
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
	bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
	void register_scroll_callback(std::function<void(int, int, int, int, float, float)> callback);
	
	void register_motion_callback(int button, std::function<void(int, int, int, int, int, int, int, bool)> callback);
	
protected:
	void draw(NVGcontext *ctx) override;

	bool mMovable;

private:
	// Queues to store events along with the button information
	std::deque<std::tuple<bool, int, int, int, int, int>> mClickQueue; // down, width, height, x, y, button
	std::deque<std::tuple<int, int, int, int, int, int, int, bool>> mMotionQueue; // width, height, x, y, dx, dy, button, dragging
	std::deque<std::tuple<int, int, int, int, float, float>> mScrollQueue; // width, height, x, y, dx, dy
	
	// Maps to store callbacks associated with specific buttons
	std::map<int, std::vector<std::function<void(bool, int, int, int, int)>>> mClickCallbacks;
	std::map<int, std::vector<std::function<void(int, int, int, int, int, int, int, bool)>>> mMotionCallbacks;
	std::deque<std::function<void(int, int, int, int, float, float)>> mScrollCallbacks;
	
	bool mDragging;
};
