#include "components/SkinnedMeshComponent.hpp"

#include "graphics/drawing/SkinnedMesh.hpp"
#include "import/ModelImporter.hpp" // Required for std::unique_ptr<ModelImporter> destructor

SkinnedMeshComponent::SkinnedMeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>&& skinnedMeshes, std::unique_ptr<ModelImporter> importer)
: mImporter(std::move(importer)), mSkinnedMeshes(std::move(skinnedMeshes)) {
	// Constructor body is empty as members are initialized in the initializer list.
}

void SkinnedMeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
										const nanogui::Matrix4f& projection) {
	// Delegate the draw call to each individual skinned mesh.
	for (auto& skinnedMesh : mSkinnedMeshes) {
		skinnedMesh->draw_content(model, view, projection);
	}
}
