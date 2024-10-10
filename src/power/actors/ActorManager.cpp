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
		// Remove the Actor from the mActors vector
		mActors.erase(it);
	} else {
		// Handle the case where the actor was not found
		// This could be a warning or exception based on your design
		throw std::runtime_error("Attempted to remove an actor that does not exist.");
	}
}

void ActorManager::remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) {
	// Step 1: Collect pointers to actors to remove
	std::unordered_set<Actor*> actors_to_remove;
	for (auto& actor_ref : actors) {
		actors_to_remove.insert(&actor_ref.get());
	}
	
	// Step 2: Verify that all actors exist in mActors
	for (Actor* actor_ptr : actors_to_remove) {
		auto it = std::find_if(mActors.begin(), mActors.end(),
							   [actor_ptr](const std::unique_ptr<Actor>& ptr) {
			return ptr.get() == actor_ptr;
		});
		if (it == mActors.end()) {
			throw std::runtime_error("Attempted to remove an actor that does not exist.");
		}
	}
	
	// Step 3: Remove the actors from mActors using std::erase_if
	std::erase_if(mActors, [&actors_to_remove](const std::unique_ptr<Actor>& ptr) {
		return actors_to_remove.find(ptr.get()) != actors_to_remove.end();
	});
}


void ActorManager::draw() {
    mCameraManager.update_view();

	for (auto& actor : mActors) {
		auto& drawable = actor.get()->get_component<DrawableComponent>();
		
		auto& transform = actor.get()->get_component<TransformComponent>();
		
		auto& color = actor.get()->get_component<ColorComponent>();
		
		color.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        nanogui::Matrix4f model = glm_to_nanogui(transform.get_matrix());

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


