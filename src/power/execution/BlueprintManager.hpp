#pragma once

#include "Canvas.hpp"
#include "simulation/SimulationServer.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>

#include <memory>

class BlueprintManager {
public:
	BlueprintManager(Canvas& canvas) : mCanvas(canvas) {
		
		mBlueprintButton = std::make_shared<nanogui::Button>(canvas, "", FA_FLASK);
		
		mBlueprintButton->set_size(nanogui::Vector2i(48, 48));
		
		// Position the button in the lower-right corner
		mBlueprintButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() * 0.5f - mBlueprintButton->width() - 10, mCanvas.fixed_height() - mBlueprintButton->height() - 10));
		
		mBlueprintButton->set_callback([this]() {
		});
	}
	
private:
	Canvas& mCanvas;
	
	std::shared_ptr<nanogui::Button> mBlueprintButton;
};
