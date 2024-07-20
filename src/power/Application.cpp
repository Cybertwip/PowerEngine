#include "Application.hpp"
#include "Canvas.hpp"
#include "ShaderManager.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "import/Fbx.hpp"

#include <nanogui/window.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>

#include <nanogui/layout.h>
#include <nanogui/icons.h>

#include <GLFW/glfw3.h>

#include <cmath>

Application::Application() : nanogui::Screen(nanogui::Vector2i(1440, 900), "NanoGUI Test", false) {
	nanogui::Window *window = new nanogui::Window(this, "Canvas widget demo");

	window->set_position(nanogui::Vector2i(15, 15));
    window->set_layout(new nanogui::GroupLayout());

    mCanvas = new Canvas(window);
	mCanvas->set_background_color({100, 100, 100, 255});
	mCanvas->set_fixed_size({400, 400});
	
	mShaderManager = std::make_unique<ShaderManager>(mCanvas->render_pass());
	
	mMeshShaderWrapper = std::make_unique<SkinnedMesh::SkinnedMeshShader>(*mShaderManager->load_shader("mesh", "shaders/simple_shader.vs", "shaders/simple_shader.fs"));
	
    mModel = std::make_unique<Fbx>("models/DeepMotionBot.fbx");
    
    for (auto& meshData : mModel->GetMeshData()) {
        mMeshes.push_back(std::make_unique<SkinnedMesh>(*meshData, *mMeshShaderWrapper));
    }
	
    Widget *tools = new Widget(window);
    tools->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 5));

	nanogui::Button *b0 = new nanogui::ToolButton(tools, FA_PLAY);
    b0->set_callback([this]() {
		mCanvas->set_background_color(nanogui::Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
    });

	nanogui::Button *b1 = new nanogui::Button(tools, "Random Rotation");
    b1->set_callback([this]() {
        //m_canvas->set_rotation((float) M_PI * rand() / (float) RAND_MAX);
    });

    perform_layout();
}

bool Application::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }
    return false;
}

void Application::draw(NVGcontext *ctx) {
    
    for (auto& mesh : mMeshes) {
        mCanvas->add_drawable(*mesh);
    }
    
    Screen::draw(ctx);
}
