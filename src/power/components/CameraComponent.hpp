#pragma once

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class TransformComponent;

class CameraComponent
{
public:
	CameraComponent(TransformComponent& transformComponent, float fov, float near, float far, float aspect);
    void update_view();
    
    const nanogui::Matrix4f& get_view() const {
        return mView;
    }
    
    const nanogui::Matrix4f& get_projection() const {
        return mProjection;
    }
	
	void look_at(Actor& actor);
private:
	TransformComponent& mTransformComponent;
	
    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
};
