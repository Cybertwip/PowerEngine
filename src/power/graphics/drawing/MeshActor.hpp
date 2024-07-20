#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include <memory>
#include <string>

class Canvas;
class Fbx;

class MeshActor : public Drawable {
public:
    MeshActor(const std::string& path, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper);
    void draw_content(CameraManager& cameraManager) override;
    
private:
    std::unique_ptr<Fbx> mModel;
    std::vector<std::unique_ptr<SkinnedMesh>> mMeshes;
};
