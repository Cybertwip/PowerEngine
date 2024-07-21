#pragma once

#include "actors/Actor.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include <entt/entt.hpp>

#include <memory>
#include <string>

class Canvas;
class Fbx;
class MeshComponent;

class MeshActor : public Actor {
public:
    MeshActor(entt::registry& registry, const std::string& path, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper);    
private:
    std::unique_ptr<Fbx> mModel;
    std::vector<std::unique_ptr<SkinnedMesh>> mMeshes;
    
    std::unique_ptr<MeshComponent> mMeshComponent;
    
};
