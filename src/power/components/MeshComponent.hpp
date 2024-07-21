#pragma once

#include "graphics/drawing/Drawable.hpp"

#include <functional>
#include <memory>
#include <vector>

class CameraManager;
class SkinnedMesh;

class MeshComponent : public Drawable {
public:
    MeshComponent(std::vector<std::reference_wrapper<SkinnedMesh>>& meshes);

    void draw_content(CameraManager& cameraManager) override;
    
private:
    std::vector<std::reference_wrapper<SkinnedMesh>> mMeshes;
};
