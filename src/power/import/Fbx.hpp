#pragma once

#include "graphics/shading/Texture.hpp"
#include "graphics/shading/MaterialProperties.hpp"

#include <SmallFBX.h>

#include <ozz/base/memory/unique_ptr.h>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

#include <glm/glm.hpp>

#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

class Fbx {
public:
    explicit Fbx(const std::string_view path);

    const ozz::animation::Skeleton& GetSkeleton() const { return *mSkeleton; }

private:
    void LoadModel(const std::string_view path);
    void ProcessNode(const std::shared_ptr<sfbx::Model> node);
    void ProcessMesh(const std::shared_ptr<sfbx::Mesh> mesh);
    void ProcessBones(const std::shared_ptr<sfbx::Mesh> mesh);

    struct Vertex {
        glm::vec3 mPosition;
        glm::vec3 mNormal;
        glm::vec2 mTexCoords1;
        glm::vec2 mTexCoords2;
        std::array<int, 4> mBoneIDs;
        std::array<float, 4> mWeights;

        Vertex() : mBoneIDs{-1, -1, -1, -1}, mWeights{0.0f, 0.0f, 0.0f, 0.0f} {}
    };

    struct Mesh {
        std::vector<Vertex> mVertices;
        std::vector<unsigned int> mIndices;
        std::vector<Texture> mTextures;
        MaterialProperties mMaterial;
    };

    std::vector<Mesh> mMeshes;
    std::unordered_map<std::string, int> mBoneMapping;
    std::vector<ozz::math::Transform> mBoneTransforms;
    ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
};
