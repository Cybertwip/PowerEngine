#include "MeshComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

MeshComponent::MeshComponent(std::vector<std::reference_wrapper<SkinnedMesh>>& meshes)
    : mMeshes(std::move(meshes)) {}

void MeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
    for (auto& mesh : mMeshes) {
        mesh.get().draw_content(model, view, projection);
    }
}
