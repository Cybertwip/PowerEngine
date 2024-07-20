#include <nanogui/nanogui.h>
#include "Application.hpp"

#include <stb_image.h>

#include <iostream>
#include <filesystem>

int main(int /* argc */, char ** /* argv */) {
	
    stbi_set_flip_vertically_on_load(1);
    
	std::cout << std::filesystem::current_path().string() << std::endl;
    try {
        nanogui::init();

        {
            nanogui::ref<Application> app = new Application();
            app->draw_all();
            app->set_visible(true);
            nanogui::mainloop(1 / 60.f * 1000);
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << std::endl;
#endif
        return -1;
    }

    return 0;
}
