#include "RenderCommon.hpp"

#include <nanogui/vector.h>

#include "Canvas.hpp"
#include "MeshActorLoader.hpp"
#include "CameraActorLoader.hpp"
#include "ShaderManager.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"

RenderCommon::RenderCommon(nanogui::Widget& parent, entt::registry& registry,
						   ActorManager& actorManager, CameraManager& cameraManager) : nanogui::Widget(parent), mActorManager(actorManager), mCameraManager(cameraManager) {
}

void RenderCommon::initialize() {
	mCanvas = std::make_shared<Canvas>(*this, nanogui::Color{70, 130, 180, 255});
	
	mCanvas->set_fixed_size(parent()->fixed_size());
	
	mShaderManager = std::make_unique<ShaderManager>(mCanvas);
	
	mShaderManager->load_default_shaders();
	
	mCameraActorLoader = std::make_unique<CameraActorLoader>(mActorManager, mCameraManager, *mShaderManager);
}
