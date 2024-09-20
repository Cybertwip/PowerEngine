#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, std::vector<std::reference_wrapper<Batch>>& batches)
    
: mActorManager(actorManager),
mMeshActorBuilder(std::make_unique<MeshActorBuilder>(batches)),
mMeshBatches(batches) {
		  
	  }

MeshActorLoader::~MeshActorLoader() {
}

const std::vector<std::reference_wrapper<Batch>>& MeshActorLoader::get_mesh_batches() {
	return mMeshBatches;
}

Actor& MeshActorLoader::create_actor(const std::string& path, ShaderWrapper& shader) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), path, shader);
}

