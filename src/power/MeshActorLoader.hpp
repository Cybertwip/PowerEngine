#pragma once

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class MeshActorBuilder;
class ShaderManager;
class ShaderWrapper;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, SkinnedMesh::MeshBatch& meshBatch);

	~MeshActorLoader();
	
    Actor& create_actor(const std::string& path, ShaderWrapper& shader);
	
	SkinnedMesh::MeshBatch& mesh_batch();

   private:
	ActorManager& mActorManager;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	
	SkinnedMesh::MeshBatch& mMeshBatch;
};
