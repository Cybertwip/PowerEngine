#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

class Actor;
class ActorManager;
class BatchUnit;
class MeshActorBuilder;
class ShaderManager;
class ShaderWrapper;

struct BatchUnit;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit);

	~MeshActorLoader();
	
    Actor& create_actor(const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	const BatchUnit& get_batch_unit();

   private:
	ActorManager& mActorManager;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	
	BatchUnit& mBatchUnit;
};
