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
		
		mBlueprintButton->set_fixed_size(nanogui::Vector2i(48, 48));
		
		mBlueprintButton->set_background_color(nanogui::Color(255, 255, 255, 255));

		mBlueprintButton->set_text_color(nanogui::Color(0, 0, 255, 255));

		// Position the button in the lower-right corner
		mBlueprintButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() * 0.5f - mBlueprintButton->fixed_width() * 0.5f, mCanvas.fixed_height() - mBlueprintButton->fixed_height() - 20));
		
		mBlueprintButton->set_callback([this]() {
		});
	}
	
private:
	Canvas& mCanvas;
	
	std::shared_ptr<nanogui::Button> mBlueprintButton;
};
