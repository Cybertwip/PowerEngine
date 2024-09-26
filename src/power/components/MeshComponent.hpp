#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class Mesh;
class SkinnedFbx;

class MeshComponent : public Drawable {
public:
    MeshComponent(std::vector<std::unique_ptr<Mesh>>&& meshes, std::unique_ptr<SkinnedFbx> model);

	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
    std::vector<std::unique_ptr<Mesh>> mMeshes;
	std::unique_ptr<SkinnedFbx> mModel;
};
