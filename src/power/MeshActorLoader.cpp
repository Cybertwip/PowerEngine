#include "MeshActorLoader.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(entt::registry& registry, ShaderManager& shaderManager)
    : mEntityRegistry(registry),
      mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)) {}

Actor& MeshActorLoader::create_mesh_actor(const std::string& path) {
    mActors.emplace_back(std::move(
        std::move(std::make_unique<MeshActor>(mEntityRegistry, path, *mMeshShaderWrapper))));
    return *mActors.back();
}
