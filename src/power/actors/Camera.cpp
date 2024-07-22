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
    auto matrix = get_component<TransformComponent>().get_matrix();
    // Convert glm::mat4 to nanogui::Matrix4f
    std::memcpy(mView.m, glm::value_ptr(matrix), sizeof(float) * 16);
}
