#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager)
    : mActorManager(actorManager),
      mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)), mMeshActorBuilder(std::make_unique<MeshActorBuilder>(*mMeshShaderWrapper)) {
		  
	  }

MeshActorLoader::~MeshActorLoader() {
}

SkinnedMesh::MeshBatch& MeshActorLoader::mesh_batch() {
	return mMeshActorBuilder->mesh_batch();
}

Actor& MeshActorLoader::create_actor(const std::string& path) {
	return mMeshActorBuilder->build(mActorManager.create_actor(), path);
}

