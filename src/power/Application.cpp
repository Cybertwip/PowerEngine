#include "Application.hpp"

#include "CameraActorLoader.hpp"
#include "CameraManager.hpp"
#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "RenderCommon.hpp"
#include "ShaderManager.hpp"
#include "UiCommon.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "ai/DeepMotionApiClient.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gizmo/GizmoManager.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"
#include "import/Fbx.hpp"
#include "ui/AnimationPanel.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/ScenePanel.hpp"
#include "ui/SceneTimeBar.hpp"
#include "ui/StatusBarPanel.hpp"
#include "ui/ResourcesPanel.hpp"
#include "ui/TransformPanel.hpp"
#include "ui/UiManager.hpp"

#include <GLFW/glfw3.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/theme.h>
#include <nanogui/window.h>

#include <cmath>
#include <functional>
Application::Application()
: nanogui::DraggableScreen("Power Engine"),
mGlobalAnimationTimeProvider(60 * 30)
{
	Batch::init_dummy_texture();
	
	mEntityRegistry = std::make_unique<entt::registry>();
	
	mCameraManager = std::make_unique<CameraManager>(*mEntityRegistry);
	
	mActorManager  = std::make_unique<ActorManager>(*mEntityRegistry, *mCameraManager);
	
	mDeepMotionApiClient = std::make_unique<DeepMotionApiClient>();
	
	mUiCommon = std::make_unique<UiCommon>(*this, *mActorManager, mGlobalAnimationTimeProvider);

	mRenderCommon = std::make_unique<RenderCommon>(mUiCommon->scene_panel(), *mEntityRegistry, *mActorManager, *mCameraManager);
	
	mMeshBatch = std::make_unique<MeshBatch>(*mRenderCommon->canvas().render_pass());
	
	mSkinnedMeshBatch = std::make_unique<SkinnedMeshBatch>(*mRenderCommon->canvas().render_pass());
	
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	
	mMeshShader = std::make_unique<ShaderWrapper>(*mRenderCommon->shader_manager().get_shader("mesh"));

	mSkinnedShader = std::make_unique<ShaderWrapper>(*mRenderCommon->shader_manager().get_shader("skinned_mesh"));
	
	mMeshActorLoader = std::make_unique<MeshActorLoader>(*mActorManager, mRenderCommon->shader_manager(), *mBatchUnit);
	
	mGizmoManager = std::make_unique<GizmoManager>(mUiCommon->toolbox(), mRenderCommon->shader_manager(), *mActorManager, *mMeshActorLoader);

	mUiManager = std::make_unique<UiManager>(
										   mUiCommon->hierarchy_panel(),
										   mUiCommon->hierarchy_panel(),
										   *mActorManager,
										   *mMeshActorLoader,
										   mRenderCommon->shader_manager(),
										   mUiCommon->scene_panel(),
										   mRenderCommon->canvas(),
										   mUiCommon->toolbox(),
										   mUiCommon->status_bar(),
										   mUiCommon->animation_panel(),
										   mUiCommon->scene_time_bar(),
										   *mCameraManager,
										   *mDeepMotionApiClient,
										   *mGizmoManager,
										   [this](std::function<void(int, int)> callback){
											   auto callbackWrapee = [this, callback](bool down, int width, int height, int x, int y){
												   callback(x, y);
											   };
											   register_click_callback(callbackWrapee);
										   }
											 );
	
	theme()->m_window_drop_shadow_size = 0;
	
	set_layout(new nanogui::GroupLayout(0, 0, 0, 0));
	
	std::vector<std::reference_wrapper<Actor>> actors;
	
	if (mCameraManager->active_camera().has_value()) {
		actors.push_back(mCameraManager->active_camera()->get());
	} else {
		actors.push_back(mRenderCommon->camera_actor_loader().create_actor(mGlobalAnimationTimeProvider,
																		   45.0f, 0.01f, 5e3f,
																		   mRenderCommon->canvas().fixed_size().x() /
																		   static_cast<float>(mRenderCommon->canvas().fixed_size().y())));
	}
	
	if (mCameraManager->active_camera().has_value()) {
		mCameraManager->active_camera()->get().get_component<TransformComponent>().set_translation(glm::vec3(0, -100, -250));
	}
	
	mUiCommon->hierarchy_panel().add_actors(std::move(actors));
	
	set_background(mRenderCommon->canvas().background_color());
	
	perform_layout();
}

bool Application::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers)) return true;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		set_visible(false);
		return true;
	}
	if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
		mUiManager->remove_active_actor();
		return true;
	}
    return false;
}

void Application::draw(NVGcontext *ctx) {
    Screen::draw(ctx);
}

void Application::process_events() {
	mGlobalAnimationTimeProvider.Update();
	
	// Dispatch queued click events
	while (!mClickQueue.empty()) {
		auto [down, w, h, x, y] = mClickQueue.front();
		mClickQueue.pop();
		
		for (auto& callback : mClickCallbacks) {
			callback(down, w, h, x, y);
		}
	}

	mUiCommon->scene_panel().process_events();
	mUiManager->status_bar_panel().resources_panel().process_events();
	
	mUiManager->process_events();
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

bool Application::drop_event(nanogui::Widget* sender, const std::vector<std::string> & filenames) {
	if (sender == &mUiManager->status_bar_panel().resources_panel()) {

		if (mUiCommon->animation_panel().contains(m_mouse_pos, true, true)) {
			if (filenames[0].find(".pan") != std::string::npos){
				mUiCommon->animation_panel().parse_file(filenames[0]);
			}
		}

		if (mRenderCommon->canvas().contains(m_mouse_pos, true, true) && !mUiManager->status_bar_panel().resources_panel().contains(m_mouse_pos, true, true)) {
			if (filenames[0].find(".psk") != std::string::npos || filenames[0].find(".pma") != std::string::npos){
				mUiCommon->hierarchy_panel().add_actor(mMeshActorLoader->create_actor(filenames[0], mGlobalAnimationTimeProvider, *mMeshShader, *mSkinnedShader));
				
				mUiCommon->scene_time_bar().refresh_actors();
			}
		}
	}
}
