#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include <cmath>

#include "Canvas.hpp"
#include "CameraManager.hpp"
#include "MeshActorLoader.hpp"
#include "RenderManager.hpp"
#include "ShaderManager.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"
#include "ui/ScenePanel.hpp"

Application::Application() : nanogui::Screen(nanogui::Vector2i(1920, 1080), "Power Engine", false) {
    
    int canvasWidth = 900;
    int canvasHeight = 600;
    
    mCameraManager = std::make_unique<CameraManager>(45.0f, 0.01f, 5e3f, canvasWidth / static_cast<float>(canvasHeight));
    mRenderManager = std::make_unique<RenderManager>(*mCameraManager);

    ScenePanel *scenePanel = new ScenePanel(this);

    mCanvas =
        std::make_unique<Canvas>(scenePanel, *mRenderManager, nanogui::Color{70, 130, 180, 255},
                                 nanogui::Vector2i{canvasWidth, canvasHeight});

    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);

    mMeshActorLoader = std::make_unique<MeshActorLoader>(*mShaderManager);

    mActors.push_back(mMeshActorLoader->create_mesh_actor("models/DeepMotionBot.fbx"));

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
    if (Screen::keyboard_event(key, scancode, action, modifiers)) return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }
    return false;
}

void Application::draw(NVGcontext *ctx) {
    for (auto &actor : mActors) {
        mRenderManager->add_drawable(*actor);
    }

    Screen::draw(ctx);
}
