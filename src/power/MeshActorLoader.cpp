#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, SkinnedMesh::MeshBatch& meshBatch)
    
: mActorManager(actorManager),
mMeshActorBuilder(std::make_unique<MeshActorBuilder>(meshBatch)),
mMeshBatch(meshBatch) {
		  
	  }

MeshActorLoader::~MeshActorLoader() {
}

SkinnedMesh::MeshBatch& MeshActorLoader::mesh_batch() {
	return mMeshBatch;
}

Actor& MeshActorLoader::create_actor(const std::string& path, ShaderWrapper& shader) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), path, shader);
}

