#include "MeshComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"

MeshComponent::MeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>& meshes) {
		for (auto& mesh : meshes) {
			mMeshes.push_back(std::move(mesh));
		}
	}

void MeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                                 const nanogui::Matrix4f& projection) {
	
    for (auto& mesh : mMeshes) {
        mesh->draw_content(model, view, projection);
    }
}

