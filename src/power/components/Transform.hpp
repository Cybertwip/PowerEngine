#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ozz/base/maths/transform.h>

class Transform {
public:
    ozz::math::Transform transform;

    Transform() {
        transform.translation = ozz::math::Float3(0.0f, 0.0f, 0.0f);
        transform.rotation = ozz::math::Quaternion::identity();
        transform.scale = ozz::math::Float3(1.0f, 1.0f, 1.0f);
    }

    void set_translation(const glm::vec3& translation) {
        transform.translation = ozz::math::Float3(translation.x, translation.y, translation.z);
    }

    void set_rotation(const glm::quat& rotation) {
        transform.rotation = ozz::math::Quaternion(rotation.w, rotation.x, rotation.y, rotation.z);
    }

    glm::vec3 get_translation() const {
        return glm::vec3(transform.translation.x, transform.translation.y, transform.translation.z);
    }

    glm::quat get_rotation() const {
        return glm::quat(transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z);
    }
};
