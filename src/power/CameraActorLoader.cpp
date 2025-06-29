#include "CameraActorLoader.hpp"

#include "CameraManager.hpp"
#include "ShaderManager.hpp"

#include "actors/ActorManager.hpp"

#include "components/CameraComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"

#include "graphics/drawing/CameraActorBuilder.hpp"

CameraActorLoader::CameraActorLoader(ActorManager& actorManager, CameraManager& cameraManager, ShaderManager& shaderManager)
: mActorManager(actorManager)
, mCameraManager(cameraManager)
, mMeshShaderWrapper(std::make_unique<ShaderWrapper>(shaderManager.get_shader("mesh"))) {}

CameraActorLoader::~CameraActorLoader() {
	
}

void CameraActorLoader::setup_engine_camera(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect) {
	
	auto& actor = mCameraManager.create_engine_camera(animationTimeProvider, fov, near, far, aspect);
	
	auto& camera = actor.get_component<CameraComponent>();
	
	camera.set_tag(CameraComponent::ECameraTag::Engine);
	
	auto& transform = actor.get_component<TransformComponent>();
	
	transform.set_translation(glm::vec3(0, 100, 250));
}

Actor& CameraActorLoader::create_actor(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect) {
	auto& actor = CameraActorBuilder::build(mActorManager.create_actor(), animationTimeProvider, fov, near, far, aspect);
	
	mCameraManager.update_from(mActorManager);
	
	return actor;

}

