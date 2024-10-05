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
#include "graphics/drawing/Batch.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"
#include "ui/UiManager.hpp"

ActorManager::ActorManager(entt::registry& registry, CameraManager& cameraManager) : mRegistry(registry), mCameraManager(cameraManager) {}

Actor& ActorManager::create_actor() {
	mActors.push_back(std::make_unique<Actor>(mRegistry));
	return *mActors.back();
}

void ActorManager::remove_actor(Actor& actor) {
	// Find the unique_ptr that owns the actor
	auto it = std::find_if(mActors.begin(), mActors.end(),
						   [&actor](const std::unique_ptr<Actor>& ptr) {
		return ptr.get() == &actor;
	});
	
	if (it != mActors.end()) {
		// Assuming Actor has a get_entity() method
		entt::entity entity = it->get().get_entity();
		
		// Destroy the entity in the registry
		if (mRegistry.valid(entity)) {
			mRegistry.destroy(entity);
		} else {
			// Handle the case where the entity is already invalid
			// This could be a warning or exception based on your design
			throw std::runtime_error("Attempted to remove an actor with an invalid entity.");
		}
		
		// Remove the Actor from the mActors vector
		mActors.erase(it);
	} else {
		// Handle the case where the actor was not found
		// This could be a warning or exception based on your design
		throw std::runtime_error("Attempted to remove an actor that does not exist.");
	}
}


void ActorManager::draw() {
    mCameraManager.update_view();

	for (auto& actor : mActors) {
		auto& drawable = actor.get()->get_component<DrawableComponent>();
		
		auto& transform = actor.get()->get_component<TransformComponent>();
		
		auto& color = actor.get()->get_component<ColorComponent>();
		
		color.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

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

void ActorManager::visit(Batch& batch) {
	mCameraManager.update_view();
	
	batch.draw_content(mCameraManager.get_view(),
						   mCameraManager.get_projection());
}


