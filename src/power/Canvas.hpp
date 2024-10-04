#pragma once

#include <nanogui/canvas.h>

#include <functional>
#include <vector>

class ActorManager;

class Canvas : public nanogui::Canvas
{
   public:
    Canvas(Widget* parent, nanogui::Color backgroundColor);

    virtual void draw_contents() override;
    
    void register_draw_callback(std::function<void()> callback);
	
	void take_snapshot(std::function<void(std::vector<uint8_t>&)> onSnapshotTaken);
	
	void process_events();


   private:
    std::vector<std::function<void()>> mDrawCallbacks;
	
	std::function<void(std::vector<uint8_t>&)> mSnapshotCallback;
};
