#include "PrimitiveComponent.hpp"

PrimitiveComponent::PrimitiveComponent(std::unique_ptr<Mesh> mesh)
: mMesh(std::move(mesh)) {
	if (!mMesh) {
		throw std::invalid_argument("Mesh cannot be null.");
	}
	
	// Optionally, you can initialize or configure the mesh here if needed.
}

void PrimitiveComponent::draw_content(const nanogui::Matrix4f& model,
									  const nanogui::Matrix4f& view,
									  const nanogui::Matrix4f& projection) {
	// Ensure the mesh exists before attempting to draw.
	if (mMesh) {
		// You can apply additional transformations or settings here if necessary.
		mMesh->draw_content(model, view, projection);
	}
}
