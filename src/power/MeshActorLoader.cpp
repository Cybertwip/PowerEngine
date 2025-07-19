#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "import/ModelImporter.hpp"

#include "simulation/PrimitiveBuilder.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit, MeshActorBuilder& actorBuilder)
    
: mActorManager(actorManager)
, mPrimitiveBuilder(std::make_unique<PrimitiveBuilder>(batchUnit.mMeshBatch))
, mBatchUnit(batchUnit)
, mMeshActorBuilder(actorBuilder){
		  
}

MeshActorLoader::~MeshActorLoader() {
}

const BatchUnit& MeshActorLoader::get_batch_unit() {
	return mBatchUnit;
}

Actor& MeshActorLoader::create_actor(const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	Actor& actor = mMeshActorBuilder.build(mActorManager.create_actor(), timeProvider, path, meshShader, skinnedShader);
	
	auto& transformComponent = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);
	
	return actor;

}

Actor& MeshActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	return mPrimitiveBuilder->build(mActorManager.create_actor(), actorName, primitiveShape, meshShader);
}
