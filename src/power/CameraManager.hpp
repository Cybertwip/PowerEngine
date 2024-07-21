#pragma once

#include <entt/entt.hpp>

#include <nanogui/vector.h>

#include <memory>

class Actor;
class Camera;

class CameraManager
{
public:
    CameraManager(entt::registry& registry);
    
    Camera& active_camera() { return mActiveCamera; }
    
    Camera& create_camera(float fov, float near, float far, float aspect);
    void update_view();
    
    const nanogui::Matrix4f& get_view() const;
    const nanogui::Matrix4f& get_projection() const;
    
    void look_at(Actor& actor);

private:
    entt::registry& mRegistry;
    Camera& mActiveCamera;
    std::vector<std::unique_ptr<Camera>> mCameras;
};
