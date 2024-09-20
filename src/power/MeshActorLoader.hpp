#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

class Actor;
class ActorManager;
class MeshActorBuilder;
class ShaderManager;
class ShaderWrapper;
class Batch;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, std::vector<std::reference_wrapper<Batch>>& batches);

	~MeshActorLoader();
	
    Actor& create_actor(const std::string& path, ShaderWrapper& shader);
	
	const std::vector<std::reference_wrapper<Batch>>& get_mesh_batches();

   private:
	ActorManager& mActorManager;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	
	std::vector<std::reference_wrapper<Batch>> mMeshBatches;
};
