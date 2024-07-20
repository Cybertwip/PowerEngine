#pragma once

#include <nanogui/vector.h>

class ShaderWrapper;

class Camera //: public Actor
{
public:
    Camera(float fov, float near, float far, float aspect);
    void set_view_projection(ShaderWrapper& shader);
    
private:
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
};
