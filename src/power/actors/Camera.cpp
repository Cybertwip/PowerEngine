#include "Camera.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "ozz/base/maths/transform.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

Camera::Camera(entt::registry& registry, float fov, float near, float far, float aspect) :
Actor(registry),
mFov(fov),
mNear(near),
mFar(far),
mAspect(aspect) {
    mProjection =
    nanogui::Matrix4f::perspective(mFov,
                                   mNear,
                                   mFar,
                                   mAspect
                                   );

    add_component<ozz::math::Transform>();
}
void Camera::set_view_projection(ShaderWrapper &shader){
    float viewOffset = -200.0f;  // Configurable parameter

    mView =
    nanogui::Matrix4f::look_at(nanogui::Vector3f(0, -2, viewOffset), nanogui::Vector3f(0, 0, 0), nanogui::Vector3f(0, 1, 0));
    
    shader.set_uniform("aView", mView);
    shader.set_uniform("aProjection", mProjection);
}

void Camera::set_position(const glm::vec3& position) {
    auto& transform = get_component<ozz::math::Transform>();
    transform.translation = { position.x, position.y, position.z };
    update_transform();
}

void Camera::set_rotation(const glm::quat& rotation) {
    auto& transform = get_component<ozz::math::Transform>();
    transform.rotation = ozz::math::Quaternion(rotation.w, rotation.x, rotation.y, rotation.z);
    update_transform();
}

void Camera::set_yaw(float yaw) {
    auto& transform = get_component<ozz::math::Transform>();
    glm::quat rotation = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));
    transform.rotation = ozz::math::Quaternion(rotation.w, rotation.x, rotation.y, rotation.z);
    update_transform();
}

void Camera::set_pitch(float pitch) {
    auto& transform = get_component<ozz::math::Transform>();
    glm::quat rotation = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
    transform.rotation = ozz::math::Quaternion(rotation.w, rotation.x, rotation.y, rotation.z);
    update_transform();
}

void Camera::set_roll(float roll) {
    auto& transform = get_component<ozz::math::Transform>();
    glm::quat rotation = glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));
    transform.rotation = ozz::math::Quaternion(rotation.w, rotation.x, rotation.y, rotation.z);
    update_transform();
}

void Camera::set_fov(float fov) {
    mFov = fov;
    mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);
}

glm::vec3 Camera::get_position() {
    auto& transform = get_component<ozz::math::Transform>();
    return glm::vec3(transform.translation.x, transform.translation.y, transform.translation.z);
}

void Camera::update_transform() {
    auto& transform = get_component<ozz::math::Transform>();
    
    // Convert ozz::math::Transform to glm types
    glm::vec3 position(transform.translation.x, transform.translation.y, transform.translation.z);
    glm::quat rotation(transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z);
    
    // Calculate the view matrix (inverse of camera's transformation)
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -position);
    glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(rotation));
    glm::mat4 view = rotationMatrix * translationMatrix;
    
    // Convert glm::mat4 to nanogui::Matrix4f
    std::memcpy(mView.m, glm::value_ptr(view), sizeof(float) * 16);
}
