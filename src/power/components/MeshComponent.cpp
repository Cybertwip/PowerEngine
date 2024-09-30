#include "MeshComponent.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "import/Fbx.hpp"

MeshComponent::MeshComponent(std::vector<std::unique_ptr<Mesh>>&& meshes, std::unique_ptr<Fbx> model) : mModel(std::move(model)) {
	mMeshes = std::move(meshes);
}

void MeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                                 const nanogui::Matrix4f& projection) {
	
    for (auto& mesh : mMeshes) {
        mesh->draw_content(model, view, projection);
    }
}

