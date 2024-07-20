#include "CameraManager.hpp"
#include "actors/Camera.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "components/Transform.hpp"

CameraManager::CameraManager(entt::registry& registry) :
mRegistry(registry),
mDefaultCamera(create_camera(45, 0.01f, 5e3f, 900.0f / 600.0f)),
mActiveCamera(mDefaultCamera) {
    mActiveCamera.get_component<Transform>().set_translation(glm::vec3(0, 0, 200));
}

Camera& CameraManager::create_camera(float fov, float near, float far, float aspect) {
    return *mCameras.emplace_back(std::make_unique<Camera>(mRegistry, fov, near, far, aspect));
}

void CameraManager::set_view_projection(ShaderWrapper &shader){
    mActiveCamera.set_view_projection(shader);
}
