#include "MeshActorLoader.hpp"

#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(ShaderManager& shaderManager)
    : mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)) {}

std::unique_ptr<MeshActor> MeshActorLoader::create_mesh_actor(const std::string& path) {
    return std::make_unique<MeshActor>(path, *mMeshShaderWrapper);
}
