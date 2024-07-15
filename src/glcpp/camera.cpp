#include "camera.h"
#include "entity/entity.h"

#include "utility.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace anim
{
CameraComponent::CameraComponent(){
	
}

void CameraComponent::initialize(std::shared_ptr<anim::Entity> owner, std::shared_ptr<anim::SharedResources> resources)
{
	front_ = glm::vec3(0.0f, 0.0f, -1.0f);
	movement_sensitivity_ = SPEED;
	angle_sensitivity_ = SENSITIVITY;
	zoom_ = ZOOM;
	_owner = owner;
	world_up_ = glm::vec3(0.0f, 1.0f, 0.0f);
}

const glm::mat4 &CameraComponent::get_view() const
{
	return view_;
}
const glm::mat4 &CameraComponent::get_projection() const
{
	return projection_;
}

const glm::vec3& CameraComponent::get_position(){
	auto component = _owner->get_component<TransformComponent>();
	
	return component->get_translation();
}

glm::vec3 CameraComponent::get_up(){
	return world_up_;
}

float CameraComponent::get_yaw(){
	auto component = _owner->get_component<TransformComponent>();
	
	return glm::degrees(component->get_rotation().y);
}

float CameraComponent::get_pitch(){
	auto component = _owner->get_component<TransformComponent>();
	
	return glm::degrees(component->get_rotation().x);
}


glm::vec3 CameraComponent::get_current_pos()
{
	glm::mat4 mat = glm::inverse(get_view());
	return mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

void CameraComponent::set_view_and_projection(float aspect)
{
	
	auto component = _owner->get_component<TransformComponent>();

	projection_ = glm::perspective(zoom_, aspect, 1.0f, 5e3f);

	glm::mat4 mat = glm::lookAt(component->get_translation(), component->get_translation() + front_, up_);
	mat = glm::rotate(mat, x_angle_, glm::vec3(1.0f, 0.0f, 0.0f));
	view_ = glm::rotate(mat, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

void CameraComponent::process_keyboard(CameraMovement direction, float deltaTime)
{
	auto component = _owner->get_component<TransformComponent>();
	float velocity = MANUAL_SPEED * deltaTime;
	if (direction == FORWARD){
		auto transformTranslation = component->get_translation() + up_ * velocity;
		
		component->set_translation(transformTranslation);

	}
	if (direction == BACKWARD){
		
		auto transformTranslation = component->get_translation() - up_ * velocity;
		component->set_translation(transformTranslation);

	}
	if (direction == LEFT){
		auto transformTranslation = component->get_translation() - right_ * velocity;
		component->set_translation(transformTranslation);
	}
	if (direction == RIGHT){
		auto transformTranslation = component->get_translation() + right_ * velocity;
		component->set_translation(transformTranslation);
	}

	update_camera_vectors();
}

void CameraComponent::process_mouse_movement(float xoffset, float yoffset)
{
	angle_sensitivity_ = 1.0f; // Adjust sensitivity as needed
	
	float x_offset_ = xoffset * angle_sensitivity_;
	float y_offset_ = yoffset * angle_sensitivity_;
	
	auto component = _owner->get_component<TransformComponent>();

	auto pitch = glm::degrees(component->get_rotation().x);
	auto yaw = glm::degrees(component->get_rotation().y);

	yaw -= y_offset_;
	pitch -= x_offset_;

	auto componentRotation = component->get_rotation();
	componentRotation.x = glm::radians(pitch);
	componentRotation.y = glm::radians(yaw);

	component->set_rotation(componentRotation);
	update_camera_vectors();
}

void CameraComponent::process_mouse_scroll(float yoffset)
{
	auto component = _owner->get_component<TransformComponent>();
	
	glm::vec3 delta = front_ * movement_sensitivity_ * 0.1f;
	if (yoffset > 0){
		auto transformTranslation = component->get_translation();
		transformTranslation += delta;
		
		component->set_translation(transformTranslation);

	}
	if (yoffset < 0){
		
		auto transformTranslation = component->get_translation();
		
		transformTranslation -= delta;
		
		component->set_translation(transformTranslation);
	}
	
	update_camera_vectors();
}

void CameraComponent::process_mouse_scroll_press(float yoffset, float xoffset, float deltaTime)
{
	auto component = _owner->get_component<TransformComponent>();
	
	float velocity = movement_sensitivity_ * deltaTime;
	auto transformTranslation = component->get_translation();
	
	transformTranslation -= up_ * velocity * yoffset;
	transformTranslation += right_ * velocity * xoffset;
	component->set_translation(transformTranslation);
	
	update_camera_vectors();
}

void CameraComponent::update_camera_vectors() {
	auto component = _owner->get_component<TransformComponent>();

	auto transform = component->get_mat4();
	
	auto [t, r, s] = DecomposeTransform(transform);

	// Assuming the default front vector points along the negative z-axis
	glm::vec3 defaultFront(0.0f, 0.0f, -1.0f);
	
	// Rotate the default front vector by the quaternion to get the new front vector
	glm::vec3 front = glm::normalize(r * defaultFront); // Normalize for safety
	
	// Use the world up vector and the new front vector to calculate the right vector
	glm::vec3 right = glm::normalize(glm::cross(front, world_up_));
	
	// Finally, calculate the up vector as the cross product of right and front vectors
	glm::vec3 up = glm::normalize(glm::cross(right, front));
	
	// Update the camera's vectors
	front_ = front;
	right_ = right;
	up_ = up;
}

}
