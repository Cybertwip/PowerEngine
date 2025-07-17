/*
 * =====================================================================================
 *
 * Filename:  ModelImporter.cpp
 *
 * Description:  Implementation of the ModelImporter class.
 *
 * =====================================================================================
 */

#include "import/ModelImporter.hpp"

// Your project's data structures
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>

namespace {
// Helper to convert Assimp's matrix to glm::mat4
glm::mat4 AssimpToGlmMat4(const aiMatrix4x4& from) {
	glm::mat4 to;
	//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}

// Decomposes a matrix into its translation, rotation, and scale components.
std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform) {
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, glm::conjugate(rotation), scale};
}
}

bool ModelImporter::LoadModel(const std::string& path) {
	mDirectory = std::filesystem::path(path).parent_path().string();
	
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path,
											 aiProcess_Triangulate |
											 aiProcess_GenSmoothNormals |
											 aiProcess_CalcTangentSpace |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_FlipUVs
											 );
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return false;
	}
	
	// Process materials first, so they are available when processing meshes.
	ProcessMaterials(scene);
	ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
	
	if(mBoneCount > 0) {
		BuildSkeleton(scene);
		ProcessAnimations(scene);
	}
	
	return true;
}

bool ModelImporter::LoadModel(const std::vector<char>& data, const std::string& formatHint) {
	mDirectory = ""; // Cannot resolve texture paths from memory
	
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(data.data(), data.size(),
													   aiProcess_Triangulate |
													   aiProcess_GenSmoothNormals |
													   aiProcess_CalcTangentSpace |
													   aiProcess_JoinIdenticalVertices |
													   aiProcess_FlipUVs,
													   formatHint.c_str()
													   );
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return false;
	}
	
	// Process materials first, so they are available when processing meshes.
	ProcessMaterials(scene);
	ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
	
	if(mBoneCount > 0) {
		BuildSkeleton(scene);
		ProcessAnimations(scene);
	}
	
	return true;
}

void ModelImporter::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
	glm::mat4 transform = parentTransform * AssimpToGlmMat4(node->mTransformation);
	
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, transform);
	}
	
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene, transform);
	}
}

void ModelImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
	std::unique_ptr<MeshData> meshData;
	std::vector<std::unique_ptr<MeshVertex>>* verticesPtr;
	
	// Create SkinnedMeshData if the mesh has bones
	if (mesh->HasBones()) {
		auto skinnedData = std::make_unique<SkinnedMeshData>();
		// We need to get the vertex vector from the derived class.
		// This is a bit tricky and relies on the memory layout.
		// A safer approach might involve a virtual get_vertices() returning a variant or base pointer.
		verticesPtr = reinterpret_cast<std::vector<std::unique_ptr<MeshVertex>>*>(&skinnedData->get_vertices());
		meshData = std::move(skinnedData);
	} else {
		meshData = std::make_unique<MeshData>();
		verticesPtr = &meshData->get_vertices();
	}
	
	auto& vertices = *verticesPtr;
	auto& indices = meshData->get_indices();
	vertices.resize(mesh->mNumVertices);
	
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(transform));
	
	// Process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		std::unique_ptr<MeshVertex> vertex;
		if (mesh->HasBones()) {
			vertex = std::make_unique<SkinnedMeshVertex>();
		} else {
			vertex = std::make_unique<MeshVertex>();
		}
		
		// Position
		glm::vec4 pos = transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
		vertex->set_position(glm::vec3(pos));
		
		// Normals
		if (mesh->HasNormals()) {
			glm::vec4 normal = normalMatrix * glm::vec4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
			vertex->set_normal(glm::normalize(glm::vec3(normal)));
		}
		
		// Texture Coordinates
		if (mesh->mTextureCoords[0]) {
			vertex->set_texture_coords1({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y});
		}
		
		// Vertex Colors
		if (mesh->mColors[0]) {
			vertex->set_color({mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a});
		}
		
		vertex->set_material_id(mesh->mMaterialIndex);
		vertices[i] = std::move(vertex);
	}
	
	// Process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}
	
	// Process bones
	if (mesh->HasBones()) {
		auto& skinnedVertices = static_cast<SkinnedMeshData*>(meshData.get())->get_vertices();
		for (unsigned int i = 0; i < mesh->mNumBones; i++) {
			aiBone* bone = mesh->mBones[i];
			std::string boneName = bone->mName.C_Str();
			int boneID;
			
			if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
				boneID = mBoneCount++;
				mBoneMapping[boneName] = boneID;
			} else {
				boneID = mBoneMapping[boneName];
			}
			
			for (unsigned int j = 0; j < bone->mNumWeights; j++) {
				unsigned int vertexID = bone->mWeights[j].mVertexId;
				float weight = bone->mWeights[j].mWeight;
				if (vertexID < skinnedVertices.size()) {
					auto* skinnedVertex = dynamic_cast<SkinnedMeshVertex*>(vertices[vertexID].get());
					if(skinnedVertex) {
						skinnedVertex->set_bone(boneID, weight);
					}
				}
			}
		}
		// Normalize weights if needed (your SkinnedMeshVertex should handle this)
	}
	
	// Assign the corresponding material to the mesh data.
	// This is possible because ProcessMaterials() was called before ProcessNode().
	if (mesh->mMaterialIndex >= 0) {
		if (static_cast<unsigned int>(mesh->mMaterialIndex) < mMaterialProperties.size()) {
			meshData->get_material_properties().push_back(mMaterialProperties[mesh->mMaterialIndex]);
		} else {
			std::cerr << "Warning: Mesh has an invalid material index: " << mesh->mMaterialIndex << std::endl;
		}
	}
	
	mMeshes.push_back(std::move(meshData));
}

void ModelImporter::ProcessMaterials(const aiScene* scene) {
	mMaterialProperties.resize(scene->mNumMaterials);
	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];
		auto matPtr = std::make_shared<MaterialProperties>();
		
		aiColor4D color;
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
			matPtr->mAmbient = {color.r, color.g, color.b, color.a};
		}
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
			matPtr->mDiffuse = {color.r, color.g, color.b, color.a};
		}
		if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS) {
			matPtr->mSpecular = {color.r, color.g, color.b, color.a};
		}
		
		float shininess, opacity;
		if (aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess) == AI_SUCCESS) {
			matPtr->mShininess = shininess;
		}
		if (aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS) {
			matPtr->mOpacity = opacity;
		}
		
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			matPtr->mTextureDiffuse = LoadMaterialTexture(material, scene, "texture_diffuse");
			matPtr->mHasDiffuseTexture = (matPtr->mTextureDiffuse != nullptr);
		}
		
		mMaterialProperties[i] = matPtr;
	}
}

std::shared_ptr<nanogui::Texture> ModelImporter::LoadMaterialTexture(aiMaterial* mat, const aiScene* scene, const std::string& typeName) {
	// Determine the Assimp texture type from our internal type name.
	aiTextureType assimpType;
	if (typeName == "texture_diffuse") {
		assimpType = aiTextureType_DIFFUSE;
	} else {
		// Extend with other types like "texture_specular" -> aiTextureType_SPECULAR if needed.
		std::cerr << "Warning: Unknown texture type requested: " << typeName << std::endl;
		return nullptr;
	}
	
	if (mat->GetTextureCount(assimpType) == 0) {
		return nullptr; // No texture of this type for the material.
	}
	
	aiString str;
	mat->GetTexture(assimpType, 0, &str);
	std::string textureIdentifier = str.C_Str();
	
	// --- 1. Try to load texture from a file path ---
	// This handles textures stored as separate files.
	std::filesystem::path pathFromFile(textureIdentifier);
	std::filesystem::path finalPath;
	
	// Check if the path is absolute and the file exists.
	if (pathFromFile.is_absolute() && std::filesystem::exists(pathFromFile)) {
		finalPath = pathFromFile;
	}
	// If not absolute, try resolving it relative to the model's directory.
	// This is the most common case for portable models.
	else if (!mDirectory.empty()) {
		finalPath = std::filesystem::path(mDirectory) / pathFromFile;
	}
	
	// If a valid path was found, try to load the file.
	if (!finalPath.empty() && std::filesystem::exists(finalPath)) {
		std::cout << "Found texture file, loading: " << finalPath << std::endl;
		std::ifstream file(finalPath, std::ios::binary);
		if (file) {
			std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
			// Create texture from the file data in memory.
			// This assumes nanogui::Texture can be constructed from a memory buffer of a PNG/JPG.
			return std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
		}
	}
	
	// --- 2. Fallback: Check for an embedded texture ---
	// This is executed if the texture was not found as a separate file.
	// Assimp's GetEmbeddedTexture can find textures referenced by an index (e.g., "*0")
	// or by their original filename.
	const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(textureIdentifier.c_str());
	if (embeddedTexture) {
		std::cout << "Could not find texture as file. Attempting to load embedded texture: " << textureIdentifier << std::endl;
		
		// The texture data is in pcData.
		// If mHeight is 0, the texture is compressed (e.g., PNG, JPG).
		// The size of the compressed data is stored in mWidth.
		if (embeddedTexture->mHeight == 0) {
			return std::make_shared<nanogui::Texture>(
													  reinterpret_cast<uint8_t*>(embeddedTexture->pcData),
													  embeddedTexture->mWidth
													  );
		} else {
			// The texture is uncompressed raw ARGB data. This requires special handling
			// that is not implemented here, as nanogui::Texture likely expects a
			// file format like PNG or JPG in its memory-based constructor.
			std::cerr << "Warning: Loading uncompressed embedded textures is not implemented for texture: " << textureIdentifier << std::endl;
			return nullptr;
		}
	}
	
	// If we reach here, the texture was not found in a file or as an embedded resource.
	std::cerr << "Warning: Failed to load texture: " << textureIdentifier << std::endl;
	return nullptr;
}

void ModelImporter::BuildSkeleton(const aiScene* scene) {
	mSkeleton = std::make_unique<Skeleton>();
	
	// Step 1: Collect offset matrices for actual bones
	std::map<std::string, glm::mat4> offsetMatrices;
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[i];
		for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
			aiBone* bone = mesh->mBones[b];
			offsetMatrices[bone->mName.C_Str()] = AssimpToGlmMat4(bone->mOffsetMatrix);
		}
	}
	
	// Step 2: Collect all bone names
	std::set<std::string> boneNames;
	for (const auto& [boneName, boneID] : mBoneMapping) {
		boneNames.insert(boneName);
	}
	
	// Step 3: Collect all relevant nodes (bones and their ancestors)
	std::set<std::string> skeletonNodes;
	for (const auto& boneName : boneNames) {
		aiNode* node = scene->mRootNode->FindNode(boneName.c_str());
		while (node) {
			skeletonNodes.insert(node->mName.C_Str());
			node = node->mParent;
		}
	}
	
	// Step 4: Add nodes to skeleton hierarchically
	AddNodeToSkeleton(scene->mRootNode, -1, scene, skeletonNodes, boneNames, offsetMatrices);
	
	// Step 5: Update mBoneMapping to reflect correct indices in the skeleton
	for (size_t i = 0; i < mSkeleton->num_bones(); ++i) {
		auto& bone = mSkeleton->get_bone(i);
		if (boneNames.find(bone.get_name()) != boneNames.end()) {
			mBoneMapping[bone.get_name()] = static_cast<int>(i);
		}
	}
}

void ModelImporter::AddNodeToSkeleton(aiNode* node, int parentBoneId, const aiScene* scene,
									  const std::set<std::string>& skeletonNodes,
									  const std::set<std::string>& boneNames,
									  const std::map<std::string, glm::mat4>& offsetMatrices) {
	std::string nodeName = node->mName.C_Str();
	if (skeletonNodes.find(nodeName) != skeletonNodes.end()) {
		// Determine if this node is an actual bone
		bool isBone = boneNames.find(nodeName) != boneNames.end();
		glm::mat4 offset = isBone ? offsetMatrices.at(nodeName) : glm::mat4(1.0f);
		glm::mat4 bindpose = AssimpToGlmMat4(node->mTransformation);
		
		// Add the node to the skeleton
		int boneId = mSkeleton->num_bones();
		mSkeleton->add_bone(nodeName, offset, bindpose, parentBoneId);
		
		// Recurse into children with this boneId as parent
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			AddNodeToSkeleton(node->mChildren[i], boneId, scene, skeletonNodes, boneNames, offsetMatrices);
		}
	} else {
		// If the node is not in skeletonNodes, still recurse into children with the same parentBoneId
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			AddNodeToSkeleton(node->mChildren[i], parentBoneId, scene, skeletonNodes, boneNames, offsetMatrices);
		}
	}
}

void ModelImporter::ProcessAnimations(const aiScene* scene) {
	for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
		aiAnimation* anim = scene->mAnimations[i];
		auto animation = std::make_unique<Animation>();
		
		double ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 24.0;
		animation->set_duration(anim->mDuration / ticksPerSecond);
		
		for (unsigned int j = 0; j < anim->mNumChannels; j++) {
			aiNodeAnim* channel = anim->mChannels[j];
			std::string boneName = channel->mNodeName.C_Str();
			
			auto it = mBoneMapping.find(boneName);
			if (it == mBoneMapping.end()) continue;
			int boneID = it->second;
			
			std::vector<Animation::KeyFrame> keyframes;
			
			// This example assumes keyframes for pos/rot/scale are aligned.
			// A more robust implementation would merge timestamps from all three tracks.
			for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
				Animation::KeyFrame frame;
				frame.time = channel->mPositionKeys[k].mTime / ticksPerSecond;
				frame.translation = {channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z};
				
				// Find corresponding rotation and scale (or interpolate)
				// For simplicity, we'll use the closest key
				unsigned int rotIndex = 0;
				for (unsigned int r = 0; r < channel->mNumRotationKeys - 1; ++r) {
					if (frame.time < channel->mRotationKeys[r + 1].mTime) {
						rotIndex = r;
						break;
					}
				}
				aiQuaternion rot = channel->mRotationKeys[rotIndex].mValue;
				frame.rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
				
				unsigned int scaleIndex = 0;
				for (unsigned int s = 0; s < channel->mNumScalingKeys - 1; ++s) {
					if (frame.time < channel->mScalingKeys[s + 1].mTime) {
						scaleIndex = s;
						break;
					}
				}
				aiVector3D scale = channel->mScalingKeys[scaleIndex].mValue;
				frame.scale = {scale.x, scale.y, scale.z};
				
				keyframes.push_back(frame);
			}
			if(!keyframes.empty()) {
				animation->add_bone_keyframes(boneID, keyframes);
			}
		}
		if (!animation->empty()) {
			mAnimations.push_back(std::move(animation));
		}
	}
}


// Accessors
std::vector<std::unique_ptr<MeshData>>& ModelImporter::GetMeshData() { return mMeshes; }
std::unique_ptr<Skeleton>& ModelImporter::GetSkeleton() { return mSkeleton; }
std::vector<std::unique_ptr<Animation>>& ModelImporter::GetAnimations() { return mAnimations; }
