#pragma once

#include "actors/Actor.hpp"

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class ShaderWrapper;

class Camera : public Actor
{
public:
    Camera(entt::registry& registry, float fov, float near, float far, float aspect);
    void set_view_projection(ShaderWrapper& shader);
    
private:
    void update_view();
    
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
};
