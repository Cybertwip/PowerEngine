#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "CameraActorLoader.hpp"
#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

RenderCommon::RenderCommon(nanogui::Widget& parent, entt::registry& registry,
						   ActorManager& actorManager, CameraManager& cameraManager) {
    mCanvas = std::make_unique<Canvas>(&parent, nanogui::Color{70, 130, 180, 255});
    mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
	mMeshActorLoader = std::make_unique<MeshActorLoader>(actorManager, *mShaderManager);
	mCameraActorLoader = std::make_unique<CameraActorLoader>(actorManager, cameraManager, *mShaderManager);
}
