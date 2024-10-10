#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/SkinnedFbx.hpp"

#include "simulation/PrimitiveBuilder.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit)
    
: mActorManager(actorManager),
mMeshActorBuilder(std::make_unique<MeshActorBuilder>(batchUnit)),
mPrimitiveBuilder(std::make_unique<PrimitiveBuilder>(batchUnit)),
mBatchUnit(batchUnit) {
		  
}

MeshActorLoader::~MeshActorLoader() {
}

const BatchUnit& MeshActorLoader::get_batch_unit() {
	return mBatchUnit;
}

Actor& MeshActorLoader::create_actor(const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), timeProvider, path, meshShader, skinnedShader);
}

Actor& MeshActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	return mPrimitiveBuilder->build(mActorManager.create_actor(), actorName, primitiveShape, meshShader);
}


