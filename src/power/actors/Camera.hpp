#pragma once

#include "actors/Actor.hpp"

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class Camera : public Actor
{
public:
    Camera(entt::registry& registry, float fov, float near, float far, float aspect);
    void update_view();
    
    const nanogui::Matrix4f& get_view() const {
        return mView;
    }
    
    const nanogui::Matrix4f& get_projection() const {
        return mProjection;
    }
    
private:
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
};
