#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/drawing/GizmoMesh.hpp"

#include <nanogui/nanogui.h>

#include <vector>
#include <memory>

class ActorManager;
class ShaderManager;

class GizmoManager : public Drawable {
public:
    GizmoManager(ShaderManager& shaderManager, ActorManager& actorManager);
    
    void draw();
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;

private:
    ShaderManager& mShaderManager;
    ActorManager& mActorManager;
    std::unique_ptr<GizmoMesh::GizmoMeshShader> mGizmoShader;
    
    std::vector<std::unique_ptr<GizmoMesh::MeshData>> mTranslationGizmoMeshData;
    std::vector<std::unique_ptr<GizmoMesh>> mTranslationGizmo;
    std::vector<nanogui::Matrix4f> mTranslationTransforms;
};

