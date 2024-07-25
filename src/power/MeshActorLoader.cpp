#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"


MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager)
    : mActorManager(actorManager),
      mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)) {}

MeshActorLoader::~MeshActorLoader() {
	// Clear the vector to invoke custom deleters
}

Actor& MeshActorLoader::create_mesh_actor(const std::string& path) {
	return MeshActorBuilder::build(mActorManager.create_actor(), path, *mMeshShaderWrapper);
}

