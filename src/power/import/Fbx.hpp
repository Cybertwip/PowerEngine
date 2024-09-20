#pragma once

#include "graphics/drawing/Mesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <SmallFBX.h>

#include <ozz/base/memory/unique_ptr.h>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

struct MeshData;

class Fbx {
public:
    explicit Fbx(const std::string& path);
	virtual ~Fbx() = default;
	
	void LoadModel();

    std::vector<std::unique_ptr<MeshData>>& GetMeshData() { return mMeshes; }
	
protected:
	virtual void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
		
	}

	sfbx::DocumentPtr mDoc;
	
	std::string mPath;

	std::vector<std::unique_ptr<MeshData>> mMeshes;

private:
    void ProcessNode(const std::shared_ptr<sfbx::Model>& node);
    void ProcessMesh(const std::shared_ptr<sfbx::Mesh>& mesh);

};
