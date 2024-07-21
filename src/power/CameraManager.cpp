#include "CameraManager.hpp"

#include "actors/Actor.hpp"
#include "actors/Camera.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

CameraManager::CameraManager(entt::registry& registry)
    : mRegistry(registry),
      mActiveCamera(create_camera(45, 0.01f, 5e3f, 900.0f / 600.0f)) {
    mActiveCamera.get_component<TransformComponent>().set_translation(glm::vec3(0, 32, 200));
}

Camera& CameraManager::create_camera(float fov, float near, float far, float aspect) {
    return *mCameras.emplace_back(std::make_unique<Camera>(mRegistry, fov, near, far, aspect));
}

void CameraManager::update_view() {
    mActiveCamera.update_view();
}

const nanogui::Matrix4f& CameraManager::get_view() const {
    return mActiveCamera.get_view();
}

const nanogui::Matrix4f& CameraManager::get_projection() const {
    return mActiveCamera.get_projection();
}

void CameraManager::look_at(Actor& actor){
    auto& cameraTransform = mActiveCamera.get_component<TransformComponent>();
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
