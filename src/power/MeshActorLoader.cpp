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

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit)
    
: mActorManager(actorManager),
mMeshActorBuilder(std::make_unique<MeshActorBuilder>(batchUnit)),
mPrimitiveBuilder(std::make_unique<PrimitiveBuilder>(batchUnit.mMeshBatch)),
mBatchUnit(batchUnit) {
		  
}

MeshActorLoader::~MeshActorLoader() {
}

const BatchUnit& MeshActorLoader::get_batch_unit() {
	return mBatchUnit;
}

Actor& MeshActorLoader::create_actor(const std::string& path, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), timeProvider, previewTimeProvider, path, meshShader, skinnedShader);
}

Actor& MeshActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	return mPrimitiveBuilder->build(mActorManager.create_actor(), actorName, primitiveShape, meshShader);
}
