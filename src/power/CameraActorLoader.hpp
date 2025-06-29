#pragma once

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class AnimationTimeProvider;
class CameraManager;
class ShaderManager;
class ShaderWrapper;

class CameraActorLoader
{
   public:
	CameraActorLoader(ActorManager& actorManager, CameraManager& cameraManager, ShaderManager& shaderManager);

	~CameraActorLoader();
	
	void setup_engine_camera(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect);
	
	Actor& create_actor(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect);

   private:
	ActorManager& mActorManager;
	CameraManager& mCameraManager;
    std::unique_ptr<ShaderWrapper> mMeshShaderWrapper;
};
