#pragma once

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class ShaderManager;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager);

	~MeshActorLoader();
	
    Actor& create_mesh_actor(const std::string& path);

   private:
	ActorManager& mActorManager;
    std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;
};
