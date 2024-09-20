// SkinnedFbxOzzAnimation.hpp
#pragma once

#include "import/SkinnedFbx.hpp"
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/animation_builder.h>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace fs = std::filesystem;

class SkinnedAnimationComponent {
public:
    SkinnedAnimationComponent(const sfbx::DocumentPtr doc,
                const std::shared_ptr<sfbx::AnimationLayer> animationLayer,
                const std::string& path);
    ~SkinnedAnimationComponent();

    void Import();

    const ozz::animation::Skeleton& GetSkeleton() const { return *skeleton_; }
    const std::vector<std::unique_ptr<ozz::animation::Animation>>& GetAnimations() const { return animations_; }

private:
    void ProcessBones(const sfbx::AnimationLayer* animationLayer, sfbx::Model* node);
    void BuildSkeleton();
    void ImportAnimations();

    ozz::math::Transform ToOzzTransform(const sfbx::float4x4& mat);

    std::shared_ptr<sfbx::Document> doc_;
    std::shared_ptr<sfbx::AnimationLayer> animationLayer_;
    std::string path_;
    float fps_;

    // Bone mappings
    std::map<std::string, int> bone_mapping_;
    std::vector<ozz::math::Transform> bone_transforms_;

    // Ozz Structures
    std::unique_ptr<ozz::animation::Skeleton> skeleton_;
    std::vector<std::unique_ptr<ozz::animation::Animation>> animations_;
};

