#include "ActorManager.hpp"

#include <nanogui/opengl.h>

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "MeshActorLoader.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"

ActorManager::ActorManager(entt::registry& registry, CameraManager& cameraManager) : mRegistry(registry), mCameraManager(cameraManager) {}

Actor& ActorManager::create_actor() {
	mActors.push_back(std::make_unique<Actor>(mRegistry));
	return *mActors.back();
}

void ActorManager::draw() {
    mCameraManager.update_view();

    for (auto& actor : mActors) {
		auto& drawable = actor.get()->get_component<DrawableComponent>();
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
