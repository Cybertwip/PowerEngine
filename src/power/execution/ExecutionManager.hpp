#pragma once

#include "Canvas.hpp"

#include "execution/BlueprintManager.hpp"
#include "simulation/SimulationServer.hpp"

#include <nanogui/button.h>
#include <nanogui/icons.h>

#include <memory>

class ExecutionManager {
public:
	enum class EExecutionMode {
		Editor,
		Game,
		Laboratory
	};
public:
	ExecutionManager(Canvas& canvas, SimulationServer& simulationServer, BlueprintManager& blueprintManager) : mCanvas(canvas)
	, mSimulationServer(simulationServer)
	, mBlueprintManager(blueprintManager)
	, mExecutionMode(EExecutionMode::Editor)
	, mSimulationTime(0) {
		
		mExecutionButton = std::make_shared<nanogui::Button>(canvas, "", FA_GAMEPAD);
		
		mExecutionButton->set_size(nanogui::Vector2i(48, 48));
		
		// Position the button in the lower-right corner
		mExecutionButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() - mExecutionButton->width() - 10, mCanvas.fixed_height() - mExecutionButton->height() - 10));
		
		mExecutionButton->set_callback([this]() {
			if (mExecutionMode == EExecutionMode::Laboratory) {
				mSimulationServer.eject();
				set_execution_mode(EExecutionMode::Game);
			} else if (mExecutionMode == EExecutionMode::Editor) {
				set_execution_mode(EExecutionMode::Game);
				mSimulationTime = 0;
				mBlueprintManager.start();
			} else if (mExecutionMode == EExecutionMode::Game) {
				set_execution_mode(EExecutionMode::Editor);
				mBlueprintManager.stop();
				mSimulationTime = 0;
			}
		});
	}
	
	void set_execution_mode(EExecutionMode executionMode) {
		if (executionMode == EExecutionMode::Game) {
			mExecutionButton->set_icon(FA_EDIT);
		} else if (executionMode == EExecutionMode::Laboratory) {
			mExecutionButton->set_icon(FA_EJECT);
		} else if (executionMode == EExecutionMode::Editor) {
			mExecutionButton->set_icon(FA_GAMEPAD);
		}
		
		mExecutionMode = executionMode;
	}
	
	void update() {
		if (mExecutionMode == EExecutionMode::Game || mExecutionMode == EExecutionMode::Laboratory) {
			mSimulationTime++; // will overflow in a few million of years. Maybe adding a two/x+-variable timer? Just in case.
		}
	}
	
private:
	Canvas& mCanvas;
	SimulationServer& mSimulationServer;
	BlueprintManager& mBlueprintManager;
	
	std::shared_ptr<nanogui::Button> mExecutionButton;
	
	EExecutionMode mExecutionMode;
	
	uint64_t mSimulationTime;
};
