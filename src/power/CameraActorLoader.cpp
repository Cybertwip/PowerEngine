#include "CameraActorLoader.hpp"

#include "CameraManager.hpp"
#include "ShaderManager.hpp"

#include "actors/ActorManager.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/CameraActorBuilder.hpp"

CameraActorLoader::CameraActorLoader(ActorManager& actorManager, CameraManager& cameraManager, ShaderManager& shaderManager)
: mActorManager(actorManager)
, mCameraManager(cameraManager)
, mMeshShaderWrapper(std::make_unique<ShaderWrapper>(shaderManager.get_shader("mesh"))) {}

CameraActorLoader::~CameraActorLoader() {
}

Actor& CameraActorLoader::create_actor(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect) {
	auto& actor = CameraActorBuilder::build(mActorManager.create_actor(), animationTimeProvider, fov, near, far, aspect);
	
	mCameraManager.update_from(mActorManager);
	
	return actor;

}

