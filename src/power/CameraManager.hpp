#pragma once

#include <nanogui/vector.h>

class ShaderWrapper;

class CameraManager
{
public:
    CameraManager(float fov, float near, float far, float aspect);
    void set_view_projection(ShaderWrapper& shader);
    
private:
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mProjection;
};
