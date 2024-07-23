#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/theme.h>
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
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

Application::Application() : nanogui::Screen(nanogui::Vector2i(1920, 1080), "Power Engine", false) {
    theme()->m_window_drop_shadow_size = 0;

    set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 1,
                                       nanogui::Alignment::Fill, 0, 0));

    mEntityRegistry = std::make_unique<entt::registry>();

    mCameraManager = std::make_unique<CameraManager>(*mEntityRegistry);

    mUiCommon = std::make_unique<UiCommon>(*this);

    mActorManager = std::make_unique<ActorManager>(*mCameraManager);

    mRenderCommon =
        std::make_unique<RenderCommon>(mUiCommon->scene_panel(), *mEntityRegistry, *mActorManager);
    mActors.push_back(
        mRenderCommon->mesh_actor_loader().create_mesh_actor("models/DeepMotionBot.fbx"));

    mActorManager->push(mActors.back());

    std::vector<std::reference_wrapper<Actor>> actors;
    actors.push_back(mActors.back());

    if (mCameraManager->active_camera().has_value()) {
        actors.push_back(mCameraManager->active_camera()->get());
    } else {
        actors.push_back(mCameraManager->create_camera(
            45.0f, 0.01f, 5e3f,
            mRenderCommon->canvas().fixed_size().x() /
                static_cast<float>(mRenderCommon->canvas().fixed_size().y())));
    }

    mCameraManager->active_camera()->get().get_component<TransformComponent>().set_translation(
        glm::vec3(0, 0, 0));

    mUiCommon->attach_actors(std::move(actors));

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
    mUiCommon->update();

    Screen::draw(ctx);
}
