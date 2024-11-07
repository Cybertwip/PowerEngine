#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class Mesh;
class Fbx;

class MeshComponent : public Drawable {
public:
    MeshComponent(std::vector<std::unique_ptr<Mesh>>&& meshes, std::unique_ptr<Fbx> model);

	virtual ~MeshComponent() = default;

	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	const std::vector<std::unique_ptr<Mesh>>& get_mesh_data() const {
		return mMeshes;
	}
    
private:
	std::unique_ptr<Fbx> mModel;
    std::vector<std::unique_ptr<Mesh>> mMeshes;
};
