#pragma once

#include <entt/entt.hpp>

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class ShaderManager;
class Actor;

class MeshActorLoader
{
   public:
    MeshActorLoader(entt::registry& registry, ShaderManager& shaderManager);

    Actor& create_mesh_actor(const std::string& path);

   private:
    std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;
    entt::registry& mEntityRegistry;
    
    std::vector<std::unique_ptr<Actor>> mActors;

};
