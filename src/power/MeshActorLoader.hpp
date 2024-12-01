#pragma once

#include "simulation/Primitive.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

class Actor;
class ActorManager;
class AnimationTimeProvider;
class BatchUnit;
class MeshActorBuilder;
class PrimitiveBuilder;
class ShaderManager;
class ShaderWrapper;

struct BatchUnit;

class MeshActorLoader
{
   public:
    MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit);

	~MeshActorLoader();
	
	Actor& create_actor(const std::string& path, AnimationTimeProvider& timeProvider,  AnimationTimeProvider& previewTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader);
	
	const BatchUnit& get_batch_unit();

   private:
	ActorManager& mActorManager;
	
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	std::unique_ptr<PrimitiveBuilder> mPrimitiveBuilder;

	BatchUnit& mBatchUnit;
};
