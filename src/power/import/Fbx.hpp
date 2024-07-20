#pragma once

#include "graphics/drawing/SkinnedMesh.hpp"

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


struct MeshData {
    std::vector<SkinnedMesh::Vertex> mVertices;
    std::vector<unsigned int> mIndices;
    std::vector<Texture> mTextures;
    MaterialProperties mMaterial;
};

class Fbx {
public:
    explicit Fbx(const std::string_view path);

    const ozz::animation::Skeleton& GetSkeleton() const { return *mSkeleton; }
    const std::vector<std::unique_ptr<MeshData>>& GetMeshData() const { return mMeshes; }

private:
    void LoadModel(const std::string_view path);
    void ProcessNode(const std::shared_ptr<sfbx::Model> node);
    void ProcessMesh(const std::shared_ptr<sfbx::Mesh> mesh);
    void ProcessBones(const std::shared_ptr<sfbx::Mesh> mesh);

    std::vector<std::unique_ptr<MeshData>> mMeshes;
    std::unordered_map<std::string, int> mBoneMapping;
    std::vector<ozz::math::Transform> mBoneTransforms;
    ozz::unique_ptr<ozz::animation::Skeleton> mSkeleton;
};
