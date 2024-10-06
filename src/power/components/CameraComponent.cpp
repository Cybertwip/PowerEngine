#include "components/CameraComponent.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"

CameraComponent::CameraComponent(TransformComponent& transformComponent, float fov, float near, float far, float aspect)
: mTransformComponent(transformComponent)
, mFov(fov)
, mNear(near)
, mFar(far)
, mAspect(aspect) {
	mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);
}

void CameraComponent::update_view() {
	glm::vec3 position = mTransformComponent.get_translation();
	glm::quat rotation = mTransformComponent.get_rotation();
	
	// Calculate forward and up vectors from rotation
	glm::vec3 forward = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 up = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
	
	// Calculate the view matrix with the rotated up vector
	glm::mat4 viewMatrix = glm::lookAt(position, position + forward, up);
	
	std::memcpy(mView.m, glm::value_ptr(viewMatrix), sizeof(float) * 16);
}

void CameraComponent::look_at(Actor& actor)
{
	auto& cameraTransform = mTransformComponent;
	auto& actorTransform = actor.get_component<TransformComponent>();
	
	glm::vec3 cameraPosition = cameraTransform.get_translation();
	glm::vec3 targetPosition = actorTransform.get_translation();
	
	glm::vec3 direction = glm::normalize(targetPosition - cameraPosition);
	
	// Assuming the up vector is the world up vector (0, 1, 0)
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	
	glm::mat4 lookAtMatrix = glm::lookAt(cameraPosition, targetPosition, up);
	glm::quat orientation = glm::quat_cast(lookAtMatrix);
	
	cameraTransform.set_rotation(orientation);
}

void CameraComponent::look_at(const glm::vec3& position)
{
	auto& cameraTransform = mTransformComponent;
	
	glm::vec3 cameraPosition = cameraTransform.get_translation();
	
	glm::vec3 direction = glm::normalize(position - cameraPosition);
	
	// Assuming the up vector is the world up vector (0, 0, 1)
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	
	glm::mat4 lookAtMatrix = glm::lookAt(cameraPosition, position, up);
	glm::quat orientation = glm::quat_cast(lookAtMatrix);
	
	cameraTransform.set_rotation(orientation);
}

void CameraComponent::set_aspect_ratio(float ratio) {
	mAspect = ratio;
	mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);
}
