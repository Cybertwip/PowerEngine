#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class SkinnedMesh;

class SkinnedMeshComponent : public Drawable {
public:
	SkinnedMeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>& skinnedMeshes);

	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
    std::vector<std::unique_ptr<SkinnedMesh>> mSkinnedMeshes;
};