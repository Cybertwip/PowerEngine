#include "components/CameraComponent.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "actors/Actor.hpp"

#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"

CameraComponent::CameraComponent(TransformComponent& transformComponent, float fov, float near, float far, float aspect, bool isDefault, bool isOrthographic)
: mTransformComponent(transformComponent)
, mFov(fov)
, mNear(near)
, mFar(far)
, mAspect(aspect)
, mActive(false)
, mDefault(isDefault)
, mOrthographic(isOrthographic) {
	update_projection();
	update_view();
}

void CameraComponent::update_projection() {
	if (mOrthographic) {
		// Define a fixed vertical size and derive width from the aspect ratio
		constexpr float orthoSize = 10.0f;
		float right = orthoSize * mAspect * 0.5f;
		float top = orthoSize * 0.5f;
		
		mProjection = nanogui::Matrix4f::ortho(-right, right, -top, top, mNear, mFar);
	} else {
		mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);
	}
}

void CameraComponent::update_view() {
	glm::vec3 position = mTransformComponent.get_translation();
	glm::quat rotation = mTransformComponent.get_rotation();
	
	// Calculate forward and up vectors from rotation
	glm::vec3 forward = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
	glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
	
	// Calculate the view matrix with the rotated up vector
	glm::mat4 viewMatrix = glm::lookAt(position, position + forward, up);
	
	std::memcpy(mView.m, glm::value_ptr(viewMatrix), sizeof(float) * 16);
}

void CameraComponent::look_at(Actor& actor)
{
	auto& actorTransform = actor.get_component<TransformComponent>();
	
	glm::vec3 targetPosition = actorTransform.get_translation();
	
	look_at(targetPosition);
}

void CameraComponent::look_at(const glm::vec3& targetPosition) {
	auto& cameraTransform = mTransformComponent;
	glm::vec3 target = targetPosition;
	glm::vec3 position = cameraTransform.get_translation();
	
	// Define the world up vector (y-up)
	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	
	// Compute the direction vector
	glm::vec3 direction = glm::normalize(target - position);
	
	if (glm::abs(glm::dot(direction, worldUp)) > 0.999f) {
		// Adjust the up vector slightly to avoid issues when looking straight up or down
		worldUp = glm::vec3(0.0f, 0.0f, -1.0f);
	}
	
	// Create the view matrix using glm::lookAt
	glm::mat4 viewMatrix = glm::lookAt(position, position + direction, worldUp);
	
	// Extract the rotation part of the view matrix
	// Note: The view matrix is the inverse of the camera's transform
	glm::mat4 cameraRotationMatrix = glm::transpose(glm::mat4(glm::mat3(viewMatrix)));
	
	// Convert the rotation matrix to a quaternion
	glm::quat orientation = glm::quat_cast(cameraRotationMatrix);
	
	// Set the camera's rotation
	cameraTransform.set_rotation(orientation);
}
