#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class SkinnedMesh;

class MeshComponent : public Drawable {
public:
    MeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>& meshes);

	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
    std::vector<std::unique_ptr<SkinnedMesh>> mMeshes;
};
