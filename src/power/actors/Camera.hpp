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
    
    // Position and rotation setters
    void set_position(const glm::vec3& position);
    void set_rotation(const glm::quat& rotation);
    void set_yaw(float yaw);
    void set_pitch(float pitch);
    void set_roll(float roll);
    void set_fov(float fov);

    // Position getter
    glm::vec3 get_position();

private:
    void update_transform();

    float mFov;
    float mNear;
    float mFar;
    float mAspect;
    nanogui::Matrix4f mView;
    nanogui::Matrix4f mProjection;
};
