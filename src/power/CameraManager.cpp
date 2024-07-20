#include "CameraManager.hpp"
#include "actors/Camera.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

CameraManager::CameraManager() :
mDefaultCamera(create_camera(45, 0.01f, 5e3f, 900.0f / 600.0f)),
mActiveCamera(mDefaultCamera) {
}

Camera& CameraManager::create_camera(float fov, float near, float far, float aspect) {
    return *mCameras.emplace_back(std::make_unique<Camera>(fov, near, far, aspect));
}

void CameraManager::set_view_projection(ShaderWrapper &shader){
    mActiveCamera.set_view_projection(shader);
}
