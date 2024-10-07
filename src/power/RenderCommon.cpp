#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "CameraActorLoader.hpp"
#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

RenderCommon::RenderCommon(std::weak_ptr<nanogui::Widget> parent, entt::registry& registry,
						   ActorManager& actorManager, CameraManager& cameraManager) {
    mCanvas = std::make_shared<Canvas>(parent, nanogui::Color{70, 130, 180, 255});
	
	mCanvas->set_fixed_size(parent.lock()->fixed_size());
	
    mShaderManager = std::make_unique<ShaderManager>(mCanvas);
	mShaderManager->load_default_shaders();
	
	mCameraActorLoader = std::make_unique<CameraActorLoader>(actorManager, cameraManager, *mShaderManager);
}
