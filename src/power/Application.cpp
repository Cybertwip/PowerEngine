#include "Application.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/theme.h>
#include <nanogui/window.h>

#include <cmath>
#include "CameraActorLoader.hpp"
#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "RenderCommon.hpp"
#include "ShaderManager.hpp"
#include "UiCommon.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"
#include "ui/UiManager.hpp"

Application::Application() : nanogui::Screen("Power Engine", true) {
		
	SkinnedMesh::init_dummy_texture();
	
    theme()->m_window_drop_shadow_size = 0;

    set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 1,
                                       nanogui::Alignment::Fill, 0, 0));

    mEntityRegistry = std::make_unique<entt::registry>();

    mCameraManager = std::make_unique<CameraManager>(*mEntityRegistry);

	mActorManager = std::make_unique<ActorManager>(*mEntityRegistry, *mCameraManager);

	mUiCommon = std::make_unique<UiCommon>(*this, *mActorManager);

	mRenderCommon =
        std::make_unique<RenderCommon>(mUiCommon->scene_panel(), *mEntityRegistry, *mActorManager, *mCameraManager);

	mMeshActorLoader = std::make_unique<MeshActorLoader>(*mActorManager, mRenderCommon->shader_manager());

	auto applicationClickCallbackRegistrator = [this](std::function<void(int, int)> callback){
		auto callbackWrapee = [this, callback](bool down, int width, int height, int x, int y){
			callback(x, y);
		};
		
		register_click_callback(callbackWrapee);
	};

	mUiManager = std::make_unique<UiManager>(mUiCommon->hierarchy_panel(), mUiCommon->hierarchy_panel(), *mActorManager, *mMeshActorLoader, mRenderCommon->shader_manager(), mUiCommon->scene_panel(), mRenderCommon->canvas(), mUiCommon->toolbox(), mUiCommon->status_bar(), *mCameraManager, applicationClickCallbackRegistrator);

//	mActors.push_back(
//	mMeshActorLoader->create_actor("models/DeepMotionBot.fbx"));
    std::vector<std::reference_wrapper<Actor>> actors;
    actors.push_back(mMeshActorLoader->create_actor("models/Venasaur/Venasaur.fbx"));

    if (mCameraManager->active_camera().has_value()) {
        actors.push_back(mCameraManager->active_camera()->get());
    } else {
		actors.push_back(mRenderCommon->camera_actor_loader().create_actor(
            45.0f, 0.01f, 5e3f,
            mRenderCommon->canvas().fixed_size().x() /
                static_cast<float>(mRenderCommon->canvas().fixed_size().y())));
    }

	if (mCameraManager->active_camera().has_value()) {
		mCameraManager->active_camera()->get().get_component<TransformComponent>().set_translation(glm::vec3(0, -50, 250));
	}
	
    mUiCommon->hierarchy_panel().add_actors(std::move(actors));
	
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
    Screen::draw(ctx);
}

void Application::process_events() {
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [down, w, h, x, y] = mClickQueue.front();
		mClickQueue.pop();
		
		for (auto& callback : mClickCallbacks) {
			callback(down, w, h, x, y);
		}
	}

	mUiCommon->scene_panel().process_events();
}

bool Application::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (button == GLFW_MOUSE_BUTTON_1 && down && !m_focused) {
		request_focus();
	}
	
	
	// Queue the click event
	if (button == GLFW_MOUSE_BUTTON_1) {
		mClickQueue.push(std::make_tuple(down, width(), height(), p.x(), p.y()));
	}
	
	return Widget::mouse_button_event(p, button, down, modifiers);
}

void Application::register_click_callback(std::function<void(bool, int, int, int, int)> callback) {
	mClickCallbacks.push_back(callback);
}
