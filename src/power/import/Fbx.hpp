#pragma once

#include "graphics/drawing/Mesh.hpp"

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

struct MeshData;

class Fbx {
public:
    explicit Fbx(const std::string_view path);

    std::vector<std::unique_ptr<MeshData>>& GetMeshData() { return mMeshes; }
protected:
	virtual void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
		
	}

private:
    void LoadModel(const std::string_view path);
    void ProcessNode(const std::shared_ptr<sfbx::Model>& node);
    void ProcessMesh(const std::shared_ptr<sfbx::Mesh>& mesh);

	std::vector<std::unique_ptr<MeshData>> mMeshes;
};
