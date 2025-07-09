#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Forward declarations to keep the header clean and reduce compile times.
// The full fbxsdk headers will be included in the .cpp file.
struct MeshData;
class MaterialProperties;

namespace fbxsdk {
class FbxManager;
class FbxImporter;
class FbxScene;
class FbxMesh;
class FbxNode;
} // namespace fbxsdk

class Fbx {
public:
	Fbx();
	virtual ~Fbx();
	
	// [FIX] A class managing raw pointers should not be copyable or movable.
	// This prevents double-freeing memory or other ownership issues.
	Fbx(const Fbx&) = delete;
	Fbx& operator=(const Fbx&) = delete;
	Fbx(Fbx&&) = delete;
	Fbx& operator=(Fbx&&) = delete;
	
	void LoadModel(std::stringstream& data);
	void LoadModel(const std::string& path);
	
	std::vector<std::unique_ptr<MeshData>>& GetMeshData();
	void SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData);
	
	std::vector<std::vector<std::shared_ptr<MaterialProperties>>>& GetMaterialProperties();
	
protected:
	// This remains for potential extension by a skinned mesh class.
	virtual void ProcessBones(fbxsdk::FbxMesh* mesh);
	
	// --- Member Variables ---
	
	std::string mFBXFilePath;
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::vector<std::vector<std::shared_ptr<MaterialProperties>>> mMaterialProperties;
	
	// [FIX] Replaced Ozz wrappers with standard FBX SDK objects.
	// These are raw pointers that this class now owns and must manage.
	fbxsdk::FbxManager* mFbxManager = nullptr;
	fbxsdk::FbxScene* mScene = nullptr;
	// The importer is short-lived and can be a local variable in LoadModel.
	// No need to store it as a member unless you have a specific reason.
	
private:
	void ProcessNode(fbxsdk::FbxNode* node);
	void ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node);
};
