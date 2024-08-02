#include "ActorManager.hpp"

#include <nanogui/opengl.h>

#include "CameraManager.hpp"
#include "MeshActorLoader.hpp"
#include "actors/Actor.hpp"
#include "components/CameraComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"
#include "ui/UiManager.hpp"

ActorManager::ActorManager(entt::registry& registry, CameraManager& cameraManager) : mRegistry(registry), mCameraManager(cameraManager) {}

Actor& ActorManager::create_actor() {
	mActors.push_back(std::make_unique<Actor>(mRegistry));
	return *mActors.back();
}

void ActorManager::draw() {
    mCameraManager.update_view();
	
	// Enable stencil test
	glEnable(GL_STENCIL_TEST);
	
	// Clear stencil buffer and depth buffer
	glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// First pass: Mark the stencil buffer
	glStencilFunc(GL_ALWAYS, 1, 0xFF);  // Always pass stencil test
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // Replace stencil buffer with 1 where actors are drawn
	glStencilMask(0xFF);  // Enable writing to the stencil buffer

	for (auto& actor : mActors) {
		auto& drawable = actor.get()->get_component<DrawableComponent>();
		auto& color = actor.get()->get_component<ColorComponent>();
		
		color.set_color(glm::vec3(1.0f, 1.0f, 1.0f));
		
		auto& transform = actor.get()->get_component<TransformComponent>();

        nanogui::Matrix4f model = TransformComponent::glm_to_nanogui(transform.get_matrix());

        drawable.draw_content(model, mCameraManager.get_view(), mCameraManager.get_projection());
    }
}

void ActorManager::visit(GizmoManager& gizmoManager) {
    mCameraManager.update_view();

    gizmoManager.draw_content(nanogui::Matrix4f::identity(), mCameraManager.get_view(),
                              mCameraManager.get_projection());
}

void ActorManager::visit(UiManager& uiManager) {
	mCameraManager.update_view();
	
	uiManager.draw_content(nanogui::Matrix4f::identity(), mCameraManager.get_view(),
							  mCameraManager.get_projection());
}

