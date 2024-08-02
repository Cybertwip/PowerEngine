#pragma once

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;

class CameraActorBuilder {
public:
	static Actor& build(Actor& actor, SkinnedMesh::SkinnedMeshShader& meshShaderWrapper, float fov, float near, float far, float aspect );
};
