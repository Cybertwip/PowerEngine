#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/drawing/Mesh.hpp"

#include <memory>

class PrimitiveComponent : public Drawable {
public:
	/**
	 * @brief Constructs a PrimitiveComponent with a single Mesh.
	 *
	 * @param mesh A unique pointer to the Mesh to be managed by this component.
	 */
	explicit PrimitiveComponent(std::unique_ptr<Mesh> mesh);
	
	/**
	 * @brief Destructor for PrimitiveComponent.
	 */
	virtual ~PrimitiveComponent() = default;
	
	/**
	 * @brief Draws the content of the mesh with the given transformation matrices.
	 *
	 * @param model The model matrix.
	 * @param view The view matrix.
	 * @param projection The projection matrix.
	 */
	void draw_content(const nanogui::Matrix4f& model,
					  const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;
	
	/**
	 * @brief Retrieves the managed Mesh.
	 *
	 * @return A constant reference to the Mesh.
	 */
	const Mesh& get_mesh() const {
		return *mMesh;
	}
	
private:
	std::unique_ptr<Mesh> mMesh;
};
