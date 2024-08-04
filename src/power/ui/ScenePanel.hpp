#pragma once

#include "Panel.hpp"

#include <queue>
#include <functional>

class ScenePanel : public Panel {
public:
    ScenePanel(nanogui::Widget& parent);
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down,
							int modifiers) override;

	void register_click_callback(std::function<void(int, int, int, int)> callback);
	
	void process_events();
	
private:
	void draw(NVGcontext *ctx) override;

	std::vector<std::function<void(int, int, int, int)>> mClickCallbacks;
	// Queue to store the click events
	std::queue<std::tuple<int, int, int, int>> mClickQueue;

};
