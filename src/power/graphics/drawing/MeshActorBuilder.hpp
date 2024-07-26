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

class MeshActorBuilder {
public:
	static Actor& build(Actor& actor, const std::string& path, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper);
};
