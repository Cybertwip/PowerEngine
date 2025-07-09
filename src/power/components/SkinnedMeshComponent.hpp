#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <memory>
#include <vector>

// Forward declarations to reduce header dependencies
class SkinnedMesh;
class ModelImporter;

/**
 * @class SkinnedMeshComponent
 * @brief A drawable component that holds and manages skinned mesh data for an actor.
 * It owns the mesh data and the ModelImporter that loaded it, ensuring data lifetime.
 */
class SkinnedMeshComponent : public Drawable {
public:
	/**
	 * @brief Constructs a SkinnedMeshComponent.
	 * @param skinnedMeshes A vector of unique pointers to the SkinnedMesh objects.
	 * @param importer A unique pointer to the ModelImporter that holds the original model data (skeleton, animations, etc.).
	 */
	SkinnedMeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>&& skinnedMeshes, std::unique_ptr<ModelImporter> importer);
	
	/**
	 * @brief Draws all the skinned meshes contained in this component.
	 */
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	/**
	 * @brief Gets a constant reference to the vector of skinned meshes.
	 */
	const std::vector<std::unique_ptr<SkinnedMesh>>& get_skinned_mesh_data() const {
		return mSkinnedMeshes;
	}
	
	/**
	 * @brief Gets a reference to the ModelImporter.
	 * This provides access to the underlying model data like animations and skeleton.
	 */
	ModelImporter& get_importer() {
		return *mImporter;
	}
	
private:
	// Owns the importer to keep all associated model data (skeleton, animations) alive.
	std::unique_ptr<ModelImporter> mImporter;
	// Owns the renderable mesh objects.
	std::vector<std::unique_ptr<SkinnedMesh>> mSkinnedMeshes;
};
