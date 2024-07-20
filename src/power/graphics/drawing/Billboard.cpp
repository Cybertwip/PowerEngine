#pragma once

#include <glm/glm.hpp>

struct MaterialProperties {
    glm::vec3 mAmbient{0.8f, 0.8f, 0.8f};
    glm::vec3 mDiffuse{1.0f, 1.0f, 1.0f};
    glm::vec3 mSpecular{0.9f, 0.9f, 0.9f};
    float mShininess{1.0f};
    float mOpacity{1.0f};
    bool mHasDiffuseTexture{false};
};
