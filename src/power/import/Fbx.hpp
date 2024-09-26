#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include "graphics/drawing/Mesh.hpp"

#include "graphics/shading/MaterialProperties.hpp"

#include <SmallFBX.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

struct MeshData;

class Fbx {
public:
	Fbx() = default;
	
    Fbx(const std::string& path);
	virtual ~Fbx() = default;
	
	void LoadModel();

	std::vector<std::unique_ptr<MeshData>>& GetMeshData() { return mMeshes; }
	
	void SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData) {
		mMeshes = std::move(meshData);
	}

	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>>& GetMaterialProperties() {
		return mMaterialProperties;
	}
	
	bool SaveTo(const std::string& filename) const;
	bool LoadFrom(const std::string& filename);
	
protected:
	virtual void ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
		
	}

	sfbx::DocumentPtr mDoc;
	
	std::string mPath;

	std::vector<std::unique_ptr<MeshData>> mMeshes;
	
	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>> mMaterialProperties;

private:
    void ProcessNode(const std::shared_ptr<sfbx::Model>& node);
    void ProcessMesh(const std::shared_ptr<sfbx::Mesh>& mesh);

};
