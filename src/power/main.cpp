#include <nanogui/nanogui.h>
#include <stb_image.h>

#include <filesystem>
#include <iostream>

#include "Application.hpp"

#include "execution/BlueprintManager.hpp"
#include "execution/ExecutionManager.hpp"
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"
#include "simulation/Cartridge.hpp"
#include "simulation/CartridgeActorLoader.hpp"
#include "simulation/SimulationServer.hpp"

int main(int /* argc */, char ** /* argv */) {
	std::cout << std::filesystem::current_path().string() << std::endl;
	nanogui::init();
	
	{
		std::shared_ptr<Application> app = std::make_shared<Application>();
		app->initialize();
		app->draw_all();
		app->set_visible(true);
		nanogui::mainloop();
	}
	
	nanogui::shutdown();
	
	return 0;
}
