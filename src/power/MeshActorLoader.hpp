#pragma once

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class MeshActorBuilder;
class ShaderManager;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager);

	~MeshActorLoader();
	
    Actor& create_actor(const std::string& path);
	
	SkinnedMesh::MeshBatch& prepared_mesh_batch();

   private:
	ActorManager& mActorManager;
    std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
};
