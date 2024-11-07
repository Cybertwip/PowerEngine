#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class SkinnedFbx;
class SkinnedMesh;

class SkinnedMeshComponent : public Drawable {
public:
	SkinnedMeshComponent(std::vector<std::unique_ptr<SkinnedMesh>>&& skinnedMeshes, std::unique_ptr<SkinnedFbx> model);

	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	const std::vector<std::unique_ptr<SkinnedMesh>>& get_skinned_mesh_data() const {
		return mSkinnedMeshes;
	}
	
	SkinnedFbx& get_model() {
		return *mModel;
	}
    
private:
	std::unique_ptr<SkinnedFbx> mModel;
    std::vector<std::unique_ptr<SkinnedMesh>> mSkinnedMeshes;	
};
