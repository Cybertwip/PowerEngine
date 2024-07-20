#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include <cmath>

#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "RenderCommon.hpp"
#include "RenderManager.hpp"
#include "ShaderManager.hpp"
#include "actors/Camera.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

Application::Application() : nanogui::Screen(nanogui::Vector2i(1920, 1080), "Power Engine", false) {
    int canvasWidth = 900;
    int canvasHeight = 600;

    mEntityRegistry = std::make_unique<entt::registry>();

    mCameraManager = std::make_unique<CameraManager>(*mEntityRegistry);
    mRenderManager = std::make_unique<RenderManager>(*mCameraManager);

    
    ScenePanel *scenePanel = new ScenePanel(this);
    TransformPanel *transformPanel = new TransformPanel(this);
    transformPanel->set_position(nanogui::Vector2i(932, 0));

    mRenderCommon = std::make_unique<RenderCommon>(*scenePanel, *mRenderManager);

    mMeshActorLoader = std::make_unique<MeshActorLoader>(mRenderCommon->shader_manager());

    mActors.push_back(mMeshActorLoader->create_mesh_actor("models/DeepMotionBot.fbx"));


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
