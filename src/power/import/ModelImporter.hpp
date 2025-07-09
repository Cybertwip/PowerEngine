#pragma once

#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>

// Forward-declare Assimp types to avoid including assimp headers in your public header.
struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

class ModelImporter {
public:
    ModelImporter() = default;
    ~ModelImporter() = default;

    // Delete copy and move constructors for simplicity
    ModelImporter(const ModelImporter&) = delete;
    ModelImporter& operator=(const ModelImporter&) = delete;

    // Main loading functions
    bool LoadModel(const std::string& path);
    bool LoadModel(const std::vector<char>& data, const std::string& formatHint);

    // Accessors for the loaded data
    std::vector<std::unique_ptr<MeshData>>& GetMeshData();
    std::unique_ptr<Skeleton>& GetSkeleton();
    std::vector<std::unique_ptr<Animation>>& GetAnimations();

private:
    // Internal processing functions
    void ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
    void ProcessMaterials(const aiScene* scene);
    void BuildSkeleton(const aiScene* scene);
	void AddNodeToSkeleton(aiNode* node,
						   int parentBoneId,
						   const aiScene* scene,
						   const std::set<std::string>& skeletonNodes,
						   const std::set<std::string>& boneNames,
						   const std::map<std::string, glm::mat4>& offsetMatrices);
    void ProcessAnimations(const aiScene* scene);

    // Helper to load textures
    std::shared_ptr<nanogui::Texture> LoadMaterialTexture(aiMaterial* mat, const aiScene* scene, const std::string& typeName);

    // Data members
    std::vector<std::unique_ptr<MeshData>> mMeshes;
    std::vector<std::shared_ptr<MaterialProperties>> mMaterialProperties;
    std::unique_ptr<Skeleton> mSkeleton;
    std::vector<std::unique_ptr<Animation>> mAnimations;

    // Bone mapping and hierarchy information
    std::map<std::string, int> mBoneMapping; // Maps bone name to its final ID
    int mBoneCount = 0;

    // Store the directory of the loaded file to find textures
    std::string mDirectory;
};
