#include "ActorManager.hpp"

#include "RenderManager.hpp"
#include "MeshActorLoader.hpp"

#include "graphics/drawing/MeshActor.hpp"

#include "components/MeshComponent.hpp"
#include "components/DrawableComponent.hpp"

#include "import/Fbx.hpp"

ActorManager::ActorManager(RenderManager& renderManager, MeshActorLoader& meshActorLoader) : mRenderManager(renderManager), mMeshActorLoader(meshActorLoader) {
    
}

Actor& ActorManager::create_mesh_actor(const std::string& path) {
    auto actor = mMeshActorLoader.create_mesh_actor(path);
    mActors.emplace_back(std::move(actor));
    return *mActors.back();
}

void ActorManager::draw(){
    for (auto &actor : mActors) {
        if (actor->find_component<DrawableComponent>()){
            mRenderManager.add_drawable(actor->get_component<DrawableComponent>());
        }
    }
}
