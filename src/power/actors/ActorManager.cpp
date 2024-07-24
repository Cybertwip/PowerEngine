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

ActorManager::ActorManager(CameraManager& cameraManager) : mCameraManager(cameraManager) {}

void ActorManager::push(Actor& actor) {
	
	auto existing = std::find_if(mActors.begin(), mActors.end(), [&actor](const Actor& iterable){
		if(&iterable == &actor){
			return true;
		} else {
			return false;
		}
	});
	
	assert(existing != mActors.end());
	
	actor.register_on_deallocated([this, &actor](){
		auto existing = std::find_if(mActors.begin(), mActors.end(), [&actor](const Actor& iterable){
			if(&iterable == &actor){
				return true;
			} else {
				return false;
			}
		});

		mActors.erase(existing);
	});
	
	mActors.push_back(actor);
}

void ActorManager::draw() {
    mCameraManager.update_view();

    for (auto& actor : mActors) {
        auto& drawable = actor.get().get_component<DrawableComponent>();
        auto& transform = actor.get().get_component<TransformComponent>();

        nanogui::Matrix4f model = TransformComponent::glm_to_nanogui(transform.get_matrix());

        drawable.draw_content(model, mCameraManager.get_view(), mCameraManager.get_projection());
    }
}

void ActorManager::visit(GizmoManager& gizmoManager) {
    mCameraManager.update_view();

    gizmoManager.draw_content(nanogui::Matrix4f::identity(), mCameraManager.get_view(),
                              mCameraManager.get_projection());
}
