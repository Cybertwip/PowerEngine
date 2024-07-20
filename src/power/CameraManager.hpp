#pragma once

#include <nanogui/vector.h>

#include <memory>

class ShaderWrapper;
class Camera;

class CameraManager
{
public:
    CameraManager();
    Camera& create_camera(float fov, float near, float far, float aspect);
    void set_view_projection(ShaderWrapper& shader);
    
private:
    Camera& mDefaultCamera;
    Camera& mActiveCamera;
    std::vector<std::unique_ptr<Camera>> mCameras;
};
