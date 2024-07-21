#pragma once

#include <memory>
#include <string>
#include <vector>

class Actor;
class MeshActor;
class MeshActorLoader;
class RenderManager;

class ActorManager {
public:
    ActorManager(RenderManager& renderManager, MeshActorLoader& meshActorLoader);
    Actor& create_mesh_actor(const std::string& path);
    void draw();
    
private:
    RenderManager& mRenderManager;
    MeshActorLoader& mMeshActorLoader;
    std::vector<std::unique_ptr<Actor>> mActors;
};

