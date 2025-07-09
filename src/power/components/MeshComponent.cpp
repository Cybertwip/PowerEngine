#include "components/MeshComponent.hpp"

#include "graphics/drawing/Mesh.hpp"
#include "import/ModelImporter.hpp" // Required for std::unique_ptr<ModelImporter> destructor

MeshComponent::MeshComponent(std::vector<std::unique_ptr<Mesh>>&& meshes, std::unique_ptr<ModelImporter> importer)
: mImporter(std::move(importer)), mMeshes(std::move(meshes)) {
	// Constructor body is empty as members are initialized in the initializer list.
}

void MeshComponent::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
								 const nanogui::Matrix4f& projection) {
	// Delegate the draw call to each individual mesh.
	for (auto& mesh : mMeshes) {
		mesh->draw_content(model, view, projection);
	}
}
