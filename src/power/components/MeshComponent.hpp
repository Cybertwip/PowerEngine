#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <memory>
#include <vector>

// Forward declarations to reduce header dependencies
class Mesh;
class ModelImporter;

/**
 * @class MeshComponent
 * @brief A drawable component that holds and manages static (non-skinned) mesh data.
 * It owns the mesh data and the ModelImporter that loaded it.
 */
class MeshComponent : public Drawable {
public:
	/**
	 * @brief Constructs a MeshComponent.
	 * @param meshes A vector of unique pointers to the Mesh objects.
	 * @param importer A unique pointer to the ModelImporter that holds the original model data.
	 */
	MeshComponent(std::vector<std::unique_ptr<Mesh>>&& meshes, std::unique_ptr<ModelImporter> importer);
	
	virtual ~MeshComponent() = default;
	
	/**
	 * @brief Draws all the meshes contained in this component.
	 */
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	/**
	 * @brief Gets a constant reference to the vector of meshes.
	 */
	const std::vector<std::unique_ptr<Mesh>>& get_mesh_data() const {
		return mMeshes;
	}
	
	/**
	 * @brief Gets a reference to the ModelImporter.
	 * This provides access to the underlying model data.
	 */
	ModelImporter& get_importer() {
		return *mImporter;
	}
	
private:
	// Owns the importer to keep all associated model data alive.
	std::unique_ptr<ModelImporter> mImporter;
	// Owns the renderable mesh objects.
	std::vector<std::unique_ptr<Mesh>> mMeshes;
};
