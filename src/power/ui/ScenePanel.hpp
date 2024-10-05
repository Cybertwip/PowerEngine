#pragma once

#include "Panel.hpp"

#include <queue>
#include <functional>
#include <map>

class ScenePanel : public Panel {
public:
	ScenePanel(nanogui::Widget& parent);
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	void register_click_callback(int button, std::function<void(bool, int, int, int, int)> callback);
	void process_events();
	
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	void register_motion_callback(int button, std::function<void(int, int, int, int, int, int, int, bool)> callback);
	
private:
	void draw(NVGcontext *ctx) override;
	
	// Queues to store events along with the button information
	std::queue<std::tuple<bool, int, int, int, int, int>> mClickQueue; // down, width, height, x, y, button
	std::queue<std::tuple<int, int, int, int, int, int, int, bool>> mMotionQueue; // width, height, x, y, dx, dy, button, dragging
	
	// Maps to store callbacks associated with specific buttons
	std::map<int, std::vector<std::function<void(bool, int, int, int, int)>>> mClickCallbacks;
	std::map<int, std::vector<std::function<void(int, int, int, int, int, int, int, bool)>>> mMotionCallbacks;
	
	bool mDragging;
};
