#pragma once

#include "Canvas.hpp"
#include "simulation/SimulationServer.hpp"

#include <memory>

class ExecutionManager {
public:
	enum class EExecutionMode {
		Game,
		Laboratory
	};
public:
	ExecutionManager(Canvas& canvas, SimulationServer& simulationServer) : mCanvas(canvas)
	, mSimulationServer(simulationServer) {
		
		mExecutionButton = std::make_shared<nanogui::Button>(canvas, "", FA_GAMEPAD);
		
		mExecutionButton->set_size(nanogui::Vector2i(48, 48));
		
		// Position the button in the lower-right corner
		mExecutionButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() - mExecutionButton->width() - 10, mCanvas.fixed_height() - mExecutionButton->height() - 10));
		
		mExecutionButton->set_callback([this]() {
			if (mExecutionMode == EExecutionMode::Laboratory) {
				mSimulationServer.eject();
				
				set_execution_mode(EExecutionMode::Game);
			}
		});
	}
	
	void set_execution_mode(EExecutionMode executionMode) {
		if (executionMode == EExecutionMode::Game) {
			mExecutionButton->set_icon(FA_GAMEPAD);
		} else if (executionMode == EExecutionMode::Laboratory) {
			mExecutionButton->set_icon(FA_FLASK);
		}
		
		mExecutionMode = executionMode;
	}
	
private:
	Canvas& mCanvas;
	SimulationServer& mSimulationServer;
	
	std::shared_ptr<nanogui::Button> mExecutionButton;
	
	EExecutionMode mExecutionMode;
};
