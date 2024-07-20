#include "Camera.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "ozz/base/maths/transform.h"


Camera::Camera(entt::registry& registry, float fov, float near, float far, float aspect) :
mFov(fov),
mNear(near),
mFar(far),
mAspect(aspect) {
    mProjection =
        nanogui::Matrix4f::perspective(mFov,
                              mNear,
                              mFar,
                              mAspect
        );
    
    registry.emplace<ozz::math::Transform>(mEntity);
}
void Camera::set_view_projection(ShaderWrapper &shader){
    static float viewOffset = -200.0f;  // Configurable parameter

    mView =
    nanogui::Matrix4f::look_at(nanogui::Vector3f(0, -2, viewOffset), nanogui::Vector3f(0, 0, 0), nanogui::Vector3f(0, 1, 0));
    
    shader.set_uniform("aView", mView);
    shader.set_uniform("aProjection", mProjection);
}
