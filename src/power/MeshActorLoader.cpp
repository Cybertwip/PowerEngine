#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "import/Fbx.hpp"


MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager)
    : mActorManager(actorManager),
      mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)) {}

MeshActorLoader::~MeshActorLoader() {
}

Actor& MeshActorLoader::create_mesh_actor(const std::string& path) {
	return MeshActorBuilder::build(mActorManager.create_actor(), path, *mMeshShaderWrapper);
}

