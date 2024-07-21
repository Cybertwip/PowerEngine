#include "MeshComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

MeshComponent::MeshComponent(std::vector<std::reference_wrapper<SkinnedMesh>>& meshes) : mMeshes(std::move(meshes)) {
    
}

void MeshComponent::draw_content(CameraManager& cameraManager) {
    for (auto& mesh : mMeshes) {
        mesh.get().draw_content(cameraManager);
    }
}
