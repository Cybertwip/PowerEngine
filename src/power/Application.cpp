#include "Application.hpp"
#include "Canvas.hpp"
#include "RenderManager.hpp"
#include "ShaderManager.hpp"
#include "graphics/shading/ShaderWrapper.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "ui/ScenePanel.hpp"

#include "import/Fbx.hpp"

#include <nanogui/window.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>

#include <nanogui/layout.h>
#include <nanogui/icons.h>

#include <GLFW/glfw3.h>

#include <cmath>

Application::Application() : nanogui::Screen(nanogui::Vector2i(1920, 1080), "Power Engine", false) {
    mRenderManager = std::make_unique<RenderManager>();

	ScenePanel *scenePanel = new ScenePanel(this);
    
    mCanvas = std::make_unique<Canvas>(scenePanel, *mRenderManager, nanogui::Color{100, 100, 100, 255}, nanogui::Vector2i{900, 600});

	
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
	
	mMeshShaderWrapper = std::make_unique<SkinnedMesh::SkinnedMeshShader>(*mShaderManager->load_shader("mesh", "shaders/simple_shader.vs", "shaders/simple_shader.fs"));
	
    mModel = std::make_unique<Fbx>("models/DeepMotionBot.fbx");
    
    for (auto& meshData : mModel->GetMeshData()) {
        mMeshes.push_back(std::make_unique<SkinnedMesh>(*meshData, *mMeshShaderWrapper));
    }
	
    Widget *tools = new Widget(scenePanel);
    tools->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 5));

	nanogui::Button *b0 = new nanogui::ToolButton(tools, FA_PLAY);
    b0->set_callback([this]() {
		//mCanvas->set_background_color(nanogui::Vector4i(rand() % 256, rand() % 256, rand() % 256, 255));
    });

	nanogui::Button *b1 = new nanogui::Button(tools, "Random Rotation");
    b1->set_callback([this]() {
        //m_canvas->set_rotation((float) M_PI * rand() / (float) RAND_MAX);
    });
    
    /* Create an empty panel with a horizontal layout */
    Widget *panel = new Widget(scenePanel);
    panel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Horizontal, nanogui::Alignment::Middle, 0, 20));

    /* Add a slider and set defaults */
    nanogui::Slider *slider = new nanogui::Slider(panel);
    slider->set_value(0.5f);
    slider->set_fixed_width(80);

    /* Add a textbox and set defaults */
    nanogui::TextBox *textBox = new nanogui::TextBox(panel);
    textBox->set_fixed_size(nanogui::Vector2i(60, 25));
    textBox->set_value("50");
    textBox->set_units("px");

    /* Propagate slider changes to the text box */
    slider->set_callback([textBox](float value) {
        textBox->set_value(std::to_string((int) (value * 100)));
    });

    nanogui::Window *propertiesWindow = new nanogui::Window(this, "Properties");
    propertiesWindow->set_position(nanogui::Vector2i(932, 0));
    propertiesWindow->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
                                                         nanogui::Alignment::Middle, 15, 5));

    new nanogui::Label(propertiesWindow, "X:", "sans-bold");
    auto int_box = new nanogui::IntBox<int>(propertiesWindow);
    int_box->set_editable(true);
    int_box->set_fixed_size(nanogui::Vector2i(100, 20));
    int_box->set_value(0);
    int_box->set_default_value("0");
    int_box->set_font_size(16);
    int_box->set_format("[1-9][0-9]*");
    int_box->set_spinnable(true);
    int_box->set_value_increment(1);

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
        mRenderManager->add_drawable(*mesh);
    }
    
    Screen::draw(ctx);
}
