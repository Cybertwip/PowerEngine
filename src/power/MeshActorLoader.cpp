#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/SkinnedFbx.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit)
    
: mActorManager(actorManager),
mMeshActorBuilder(std::make_unique<MeshActorBuilder>(batchUnit)),
mBatchUnit(batchUnit) {
		  
	  }

MeshActorLoader::~MeshActorLoader() {
}

const BatchUnit& MeshActorLoader::get_batch_unit() {
	return mBatchUnit;
}

Actor& MeshActorLoader::create_actor(const std::string& path, ShaderWrapper& shader) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), path, shader);
}

