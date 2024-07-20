#include "graphics/drawing/MeshActor.hpp"

#include "import/Fbx.hpp"

MeshActor::MeshActor(const std::string& path, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper) {
    mModel = std::make_unique<Fbx>("models/DeepMotionBot.fbx");

    for (auto& meshData : mModel->GetMeshData()) {
        mMeshes.push_back(std::make_unique<SkinnedMesh>(*meshData, meshShaderWrapper));
    }
}

void MeshActor::draw_content(CameraManager& cameraManager) {
    for (auto& mesh : mMeshes) {
        mesh->draw_content(cameraManager);
    }
}
