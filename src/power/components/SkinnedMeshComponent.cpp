#include "SkinnedMeshComponent.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"
#include "import/ModelImporter.hpp"

SkinnedMeshComponent::SkinnedMeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>&& skinnedMeshes, std::unique_ptr<SkinnedFbx> model) : mModel(std::move(model)) {
	mSkinnedMeshes = std::move(skinnedMeshes);
}

void SkinnedMeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
                                 const nanogui::Matrix4f& projection) {
	
    for (auto& skinnedMesh : mSkinnedMeshes) {
		skinnedMesh->draw_content(model, view, projection);
    }
}

