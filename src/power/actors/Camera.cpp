#include "Camera.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

Camera::Camera(entt::registry& registry, float fov, float near, float far, float aspect)
    : Actor(registry), mFov(fov), mNear(near), mFar(far), mAspect(aspect) {
    mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);

    add_component<TransformComponent>();
    add_component<MetadataComponent>("Camera");
}

void Camera::update_view() {
    auto& transform = get_component<TransformComponent>();

    // Convert ozz::math::Transform to glm types
    glm::vec3 position = transform.get_translation();
    glm::quat rotation = transform.get_rotation();

    // Calculate the view matrix (inverse of camera's transformation)
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -position);
    glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(rotation));
    glm::mat4 view = rotationMatrix * translationMatrix;

    // Convert glm::mat4 to nanogui::Matrix4f
    std::memcpy(mView.m, glm::value_ptr(view), sizeof(float) * 16);
}
