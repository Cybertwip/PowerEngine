#include "PrimitiveComponent.hpp"

PrimitiveComponent::PrimitiveComponent(std::unique_ptr<Mesh> mesh, std::unique_ptr<MeshData> meshData)
: mMeshData(std::move(meshData)), mMesh(std::move(mesh)) {
	// Optionally, you can initialize or configure the mesh here if needed.
}

void PrimitiveComponent::draw_content(const nanogui::Matrix4f& model,
									  const nanogui::Matrix4f& view,
									  const nanogui::Matrix4f& projection) {
	mMesh->draw_content(model, view, projection);
}
