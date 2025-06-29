#pragma once

#include "filesystem/CompressedSerialization.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "ozz/animation/offline/fbx/fbx.h"
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>

struct MeshData;

namespace fbxsdk {
class FbxMesh;
class FbxNode;
class FbxScene;
}

class Fbx {
public:
	Fbx() = default;
	virtual ~Fbx() = default;
	
	void LoadModel(std::stringstream& data);
	void LoadModel(const std::string& path);
	
	std::vector<std::unique_ptr<MeshData>>& GetMeshData();
	void SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData);
	
	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>>& GetMaterialProperties();
	
	bool SaveTo(const std::string& filename) const;
	bool LoadFrom(const std::string& filename);
	
protected:
	virtual void ProcessBones(fbxsdk::FbxMesh* mesh);
	
	std::string mPath;
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>> mMaterialProperties;
	
	std::unique_ptr<ozz::animation::offline::fbx::FbxManagerInstance> mFbxManager;
	std::unique_ptr<ozz::animation::offline::fbx::FbxDefaultIOSettings> mSettings;
	std::unique_ptr<ozz::animation::offline::fbx::FbxSceneLoader> mSceneLoader;
	
private:
	void ProcessNode(fbxsdk::FbxNode* node, const glm::mat4& parentTransform);
	void ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node, const glm::mat4& transform);
	
	std::string mFBXFilePath;
};
