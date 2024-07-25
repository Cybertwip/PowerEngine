#pragma once

#include "actors/Actor.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/drawing/GizmoMesh.hpp"

#include <entt/entt.hpp>
#include <memory>
#include <string>

class Canvas;
class Fbx;
class GizmoComponent;

class GizmoActor : public Actor {
public:
    GizmoActor(entt::registry& registry, const std::string& path, GizmoMesh::GizmoMeshShader& gizmoShaderWrapper);
private:
    std::unique_ptr<Fbx> mModel;
    std::vector<std::unique_ptr<GizmoMesh>> mGizmos;    
};
