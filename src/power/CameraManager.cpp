#include "CameraManager.hpp"

#include "actors/Actor.hpp"
#include "actors/Camera.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

CameraManager::CameraManager(entt::registry& registry)
    : mRegistry(registry), mActiveCamera(std::nullopt) {
}

Camera& CameraManager::create_camera(float fov, float near, float far, float aspect) {
    auto camera = std::make_unique<Camera>(mRegistry, fov, near, far, aspect);
    
    if (mActiveCamera == std::nullopt) {
        mActiveCamera = *camera;
    }
    
    mCameras.push_back(std::move(camera));
    
    return *mCameras.back();
}

void CameraManager::update_view() {
    if (mActiveCamera.has_value()) {
        mActiveCamera->get().update_view();
    }
}

const nanogui::Matrix4f CameraManager::get_view() const {
    if (mActiveCamera.has_value()) {
        return mActiveCamera->get().get_view();
    } else {
        return nanogui::Matrix4f();
    }
}
const nanogui::Matrix4f CameraManager::get_projection() const {
    if (mActiveCamera.has_value()) {
        return mActiveCamera->get().get_projection();
    } else {
        return nanogui::Matrix4f::perspective(45, 0.01f, 5e3f, 16.0f / 9.0f);
    }
}

void CameraManager::look_at(Actor& actor) {
    
    if (mActiveCamera.has_value()) {
        auto& cameraTransform = mActiveCamera->get().get_component<TransformComponent>();
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

}
