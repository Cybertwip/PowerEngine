#pragma once

#include <memory>
#include <string>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class CameraManager;
class ShaderManager;

class CameraActorLoader
{
   public:
	CameraActorLoader(ActorManager& actorManager, CameraManager& cameraManager, ShaderManager& shaderManager);

	~CameraActorLoader();
	
	
	Actor& create_actor(float fov, float near, float far, float aspect);

   private:
	ActorManager& mActorManager;
	CameraManager& mCameraManager;
    std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;
};
