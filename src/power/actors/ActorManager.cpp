#include "ActorManager.hpp"

#include <unordered_set>

#include "CameraManager.hpp"
#include "MeshActorLoader.hpp"
// INTEGRATION: Include the serializer
#include "serialization/SceneSerializer.hpp"
#include "serialization/UUID.hpp"

#include "actors/Actor.hpp"
#include "components/CameraComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "gizmo/GizmoManager.hpp"
#include "graphics/drawing/Batch.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/ModelImporter.hpp"
#include "ui/UiManager.hpp"

ActorManager::ActorManager(entt::registry& registry, CameraManager& cameraManager) : mRegistry(registry), mCameraManager(cameraManager) {}

Actor& ActorManager::create_actor() {
    // This correctly creates an Actor which in turn creates an entity and adds an IDComponent
    mActors.push_back(std::make_unique<Actor>(mRegistry));
    return *mActors.back();
}

Actor& ActorManager::create_actor(entt::entity entity) {
    // This correctly creates an Actor which in turn creates an entity and adds an IDComponent
    mActors.push_back(std::make_unique<Actor>(mRegistry, entity));
    return *mActors.back();
}

void ActorManager::remove_actor(Actor& actor) {
    // The actor's destructor will handle destroying the entt::entity.
    // We just need to remove the manager's handle to it.
    auto it = std::find_if(mActors.begin(), mActors.end(),
                           [&actor](const std::unique_ptr<Actor>& ptr) {
        return ptr.get() == &actor;
    });
    
    if (it != mActors.end()) {
        mActors.erase(it);
    } else {
        throw std::runtime_error("Attempted to remove an actor that does not exist in the manager.");
    }
}

void ActorManager::remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) {
    std::unordered_set<Actor*> actors_to_remove;
    for (const auto& actor_ref : actors) {
        actors_to_remove.insert(&actor_ref.get());
    }
    
    // The actors' destructors will be called automatically when they are erased,
    // which will in turn destroy their associated entt::entity.
    std::erase_if(mActors, [&actors_to_remove](const std::unique_ptr<Actor>& ptr) {
        return actors_to_remove.count(ptr.get());
    });
}


void ActorManager::draw() {
    mCameraManager.update_view();

    // This logic is fine, but be aware it will throw an exception if an actor
    // is missing a required component.
    for (auto& actor : mActors) {
        if (actor->find_component<DrawableComponent>() &&
            actor->find_component<TransformComponent>() &&
            actor->find_component<ColorComponent>())
        {
            auto& drawable = actor->get_component<DrawableComponent>();
            auto& transform = actor->get_component<TransformComponent>();
            auto& color = actor->get_component<ColorComponent>();
            
            color.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            nanogui::Matrix4f model = glm_to_nanogui(transform.get_matrix());

            drawable.draw_content(model, mCameraManager.get_view(), mCameraManager.get_projection());
        }
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

void ActorManager::clear_actors() {
    // First, clear the vector of Actor wrappers. This will call their destructors,
    // which in turn will destroy all entities in the registry.
    mActors.clear();
    // Finally, ensure the registry itself is cleared of any leftover data.
    mRegistry.clear();
}

