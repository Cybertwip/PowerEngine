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
#include "ShaderManager.hpp"
#include "UiCommon.hpp"
#include "actors/ActorManager.hpp"
#include "actors/Camera.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

Application::Application() : nanogui::Screen(nanogui::Vector2i(1920, 1080), "Power Engine", false) {
    set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
                                       nanogui::Alignment::Fill, 15, 5));

    mEntityRegistry = std::make_unique<entt::registry>();

    mCameraManager = std::make_unique<CameraManager>(*mEntityRegistry);

    mRenderSettings = std::make_unique<RenderSettings>(900, 600);
    mUiCommon = std::make_unique<UiCommon>(*this);

    mActorManager = std::make_unique<ActorManager>(*mCameraManager);

    mRenderCommon = std::make_unique<RenderCommon>(mUiCommon->scene_panel(), *mEntityRegistry,
                                                   *mActorManager, *mRenderSettings);

    mActors.push_back(
        mRenderCommon->mesh_actor_loader().create_mesh_actor("models/DeepMotionBot.fbx"));

    mActorManager->push(mActors.back());

    std::vector<Actor *> actors;
    for (int i = 0; i < 100; ++i) {
        actors.push_back(&(mActors.back().get()));
    }
    mUiCommon->hierarchy_panel().set_actors(actors);

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
    mUiCommon->transform_panel().gather_values_into(
        mActors[0].get().get_component<TransformComponent>());

    //    mUiCommon->transform_panel().gather_values_into(
    //                                                    mCameraManager->active_camera().get_component<TransformComponent>());

    mCameraManager->look_at(mActors.back());

    // mActorManager->draw(); // looks cool but not logic

    Screen::draw(ctx);
}
