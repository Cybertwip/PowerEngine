#pragma once

#include "animation/AnimationTimeProvider.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;

class CameraActorBuilder {
public:
	static Actor& build(Actor& actor, AnimationTimeProvider&, ShaderWrapper& meshShaderWrapper, float fov, float near, float far, float aspect );
};
