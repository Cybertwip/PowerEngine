#pragma once

#include <entt/entt.hpp>

#include <nanogui/vector.h>

#include <memory>

class ShaderWrapper;
class Camera;

class CameraManager
{
public:
    CameraManager(entt::registry& registry);
    Camera& create_camera(float fov, float near, float far, float aspect);
    void set_view_projection(ShaderWrapper& shader);
    
private:
    entt::registry& mRegistry;
    Camera& mDefaultCamera;
    Camera& mActiveCamera;
    std::vector<std::unique_ptr<Camera>> mCameras;
};
