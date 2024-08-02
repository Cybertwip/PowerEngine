#pragma once

#include "graphics/drawing/SkinnedMesh.hpp"

#include <string>

class Actor;
class MeshComponent;

class MeshActorBuilder {
public:
	static Actor& build(Actor& actor, const std::string& path, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper);
};
