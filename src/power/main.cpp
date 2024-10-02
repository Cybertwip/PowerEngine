#include <nanogui/nanogui.h>
#include <stb_image.h>

#include <filesystem>
#include <iostream>

#include "Application.hpp"

int main(int /* argc */, char ** /* argv */) {
	stbi_set_flip_vertically_on_load(1);
	
	std::cout << std::filesystem::current_path().string() << std::endl;
	nanogui::init();
	
	{
		nanogui::ref<Application> app = new Application();
		app->draw_all();
		app->set_visible(true);
		nanogui::mainloop();
	}
	
	nanogui::shutdown();
	
	return 0;
}
