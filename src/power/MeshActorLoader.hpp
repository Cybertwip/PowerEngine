#pragma ocne

#include "graphics/drawing/SkinnedMesh.hpp"

#include <memory>
#include <string>

class ShaderManager;
class MeshActor;


class MeshActorLoader {
public:
    MeshActorLoader(ShaderManager& shaderManager);
    
    std::unique_ptr<MeshActor> create_mesh_actor(const std::string& path);
    
private:
    std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;
};
