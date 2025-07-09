#include "import/ModelImporter.hpp"

// Your project's data structures
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "animation/Skeleton.hpp" // Assuming this is where Skeleton is defined
#include "animation/Animation.hpp" // Assuming this is where Animation is defined

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// GLM includes
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

// Standard library includes
#include <iostream>
#include <filesystem>
#include <map>
#include <set>

namespace {
/**
 * @brief Converts an Assimp aiMatrix4x4 to a GLM mat4.
 * Assimp matrices are row-major, while GLM matrices are column-major.
 * This function handles the necessary transposition.
 * @param from The Assimp matrix.
 * @return The equivalent GLM matrix.
 */
glm::mat4 AssimpToGlmMat4(const aiMatrix4x4& from) {
	// A direct memory copy and transpose is the cleanest way.
	return glm::transpose(glm::make_mat4(&from.a1));
}

/**
 * @brief Converts an Assimp aiVector3D to a GLM vec3.
 * @param vec The Assimp vector.
 * @return The equivalent GLM vector.
 */
glm::vec3 AssimpToGlmVec3(const aiVector3D& vec) {
	return {vec.x, vec.y, vec.z};
}

/**
 * @brief Converts an Assimp aiQuaternion to a GLM quat.
 * Note the order of arguments: GLM's constructor is (w, x, y, z).
 * @param p_quat The Assimp quaternion.
 * @return The equivalent GLM quaternion.
 */
glm::quat AssimpToGlmQuat(const aiQuaternion& p_quat) {
	return {p_quat.w, p_quat.x, p_quat.y, p_quat.z};
}
}

// --- Primary Loading Functions ---

bool ModelImporter::LoadModel(const std::string& path) {
	// Set the directory for resolving texture paths
	mDirectory = std::filesystem::path(path).parent_path().string();
	
	Assimp::Importer importer;
	// Define the post-processing flags for Assimp.
	// aiProcess_GlobalScale is deprecated and often not needed.
	// aiProcess_PopulateArmatureData is a new, better way to get skeleton info.
	const unsigned int flags =
	aiProcess_Triangulate |
	aiProcess_GenSmoothNormals |
	aiProcess_CalcTangentSpace |
	aiProcess_JoinIdenticalVertices |
	aiProcess_FlipUVs | // Often needed for OpenGL
	aiProcess_ValidateDataStructure |
	aiProcess_PopulateArmatureData; // Essential for modern skeleton processing
	
	const aiScene* scene = importer.ReadFile(path, flags);
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return false;
	}
	
	// Process the model data
	return ProcessScene(scene);
}

bool ModelImporter::LoadModel(const std::vector<char>& data, const std::string& formatHint) {
	mDirectory = ""; // Cannot resolve texture paths when loading from memory
	
	Assimp::Importer importer;
	const unsigned int flags =
	aiProcess_Triangulate |
	aiProcess_GenSmoothNormals |
	aiProcess_CalcTangentSpace |
	aiProcess_JoinIdenticalVertices |
	aiProcess_FlipUVs |
	aiProcess_ValidateDataStructure |
	aiProcess_PopulateArmatureData;
	
	const aiScene* scene = importer.ReadFileFromMemory(data.data(), data.size(), flags, formatHint.c_str());
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return false;
	}
	
	return ProcessScene(scene);
}

bool ModelImporter::ProcessScene(const aiScene* scene) {
	// The order of processing is important.
	// 1. Materials, as meshes will reference them by index.
	ProcessMaterials(scene);
	
	// 2. Meshes and Bones. This populates the bone mapping needed for the skeleton.
	ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
	
	// 3. Skeleton and Animations, which depend on the bone mapping created above.
	if (mBoneCount > 0) {
		BuildSkeleton(scene);
		ProcessAnimations(scene);
	}
	
	return true;
}


// --- Mesh and Node Processing ---

void ModelImporter::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
	// Accumulate transform from parent
	glm::mat4 transform = parentTransform * AssimpToGlmMat4(node->mTransformation);
	
	// Process all the node's meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, transform);
	}
	
	// Then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene, transform);
	}
}

void ModelImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
	// This is the correct, safe way to handle polymorphism with vectors of unique_ptr.
	// We create a vector of the base type, and emplace_back pointers to derived types.
	std::vector<std::unique_ptr<MeshVertex>> vertices;
	vertices.reserve(mesh->mNumVertices);
	
	std::vector<unsigned int> indices;
	
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(transform));
	
	// Process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		// Create the appropriate vertex type.
		// We emplace it directly into the vector later.
		std::unique_ptr<MeshVertex> vertex;
		if (mesh->HasBones()) {
			vertex = std::make_unique<SkinnedMeshVertex>();
		} else {
			vertex = std::make_unique<MeshVertex>();
		}
		
		// Position (transformed into world space)
		glm::vec4 pos = transform * glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
		vertex->set_position(glm::vec3(pos));
		
		// Normals (transformed correctly)
		if (mesh->HasNormals()) {
			glm::vec4 normal = normalMatrix * glm::vec4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
			vertex->set_normal(glm::normalize(glm::vec3(normal)));
		}
		
		// Texture Coordinates (only channel 0 is processed here)
		if (mesh->mTextureCoords[0]) {
			vertex->set_texture_coords1({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y});
		}
		
		// Vertex Colors (only channel 0 is processed here)
		if (mesh->mColors[0]) {
			vertex->set_color({mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a});
		}
		
		vertex->set_material_id(mesh->mMaterialIndex);
		vertices.emplace_back(std::move(vertex));
	}
	
	// Process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}
	
	// Create the final MeshData object.
	std::unique_ptr<MeshData> meshData;
	if (mesh->HasBones()) {
		auto skinnedData = std::make_unique<SkinnedMeshData>();
		
		// Process bones and apply weights to the vertices
		for (unsigned int i = 0; i < mesh->mNumBones; i++) {
			aiBone* bone = mesh->mBones[i];
			std::string boneName = bone->mName.C_Str();
			int boneID;
			
			if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
				boneID = mBoneCount++;
				mBoneMapping[boneName] = boneID;
				
				// Store the inverse bind matrix (offset matrix) provided by Assimp
				mBoneInfo.emplace_back(AssimpToGlmMat4(bone->mOffsetMatrix));
			} else {
				boneID = mBoneMapping[boneName];
			}
			
			for (unsigned int j = 0; j < bone->mNumWeights; j++) {
				unsigned int vertexID = bone->mWeights[j].mVertexId;
				float weight = bone->mWeights[j].mWeight;
				if (vertexID < vertices.size()) {
					// Safely downcast to set bone data
					auto* skinnedVertex = static_cast<SkinnedMeshVertex*>(vertices[vertexID].get());
					skinnedVertex->add_bone_data(boneID, weight);
				}
			}
		}
		
		// Move the correctly populated vertex vector into the SkinnedMeshData
		skinnedData->set_vertices(std::move(vertices));
		meshData = std::move(skinnedData);
	} else {
		meshData = std::make_unique<MeshData>();
		meshData->set_vertices(std::move(vertices));
	}
	
	meshData->set_indices(std::move(indices));
	mMeshes.push_back(std::move(meshData));
}

// --- Skeleton Building ---

void ModelImporter::BuildSkeleton(const aiScene* scene) {
	mSkeleton = std::make_unique<Skeleton>();
	// Start the recursive build from the scene's root node.
	// This correctly preserves the parent-child hierarchy.
	AddBoneToSkeletonRecursive(scene->mRootNode, -1);
}

void ModelImporter::AddBoneToSkeletonRecursive(aiNode* node, int parentID) {
	std::string nodeName = node->mName.C_Str();
	int currentBoneID = -1;
	
	// Check if the current node is a bone that affects our meshes
	auto it = mBoneMapping.find(nodeName);
	if (it != mBoneMapping.end()) {
		// This node is a bone in our skeleton
		currentBoneID = it->second;
		glm::mat4 offsetMatrix = mBoneInfo[currentBoneID].offsetMatrix;
		
		// The bind pose is the node's local transform
		glm::mat4 bindPose = AssimpToGlmMat4(node->mTransformation);
		
		mSkeleton->add_bone(nodeName, offsetMatrix, bindPose, parentID);
	} else {
		// This node is not a bone, but could be an intermediate transform node.
		// We pass its parent's ID down to its children.
		currentBoneID = parentID;
	}
	
	// Recurse for all children
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		AddBoneToSkeletonRecursive(node->mChildren[i], currentBoneID);
	}
}


// --- Animation Processing ---

void ModelImporter::ProcessAnimations(const aiScene* scene) {
	for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
		aiAnimation* assimpAnimation = scene->mAnimations[i];
		auto animation = std::make_unique<Animation>();
		
		double ticksPerSecond = assimpAnimation->mTicksPerSecond != 0 ? assimpAnimation->mTicksPerSecond : 25.0;
		animation->set_duration(assimpAnimation->mDuration / ticksPerSecond);
		animation->set_name(assimpAnimation->mName.C_Str());
		
		for (unsigned int j = 0; j < assimpAnimation->mNumChannels; j++) {
			aiNodeAnim* channel = assimpAnimation->mChannels[j];
			std::string boneName = channel->mNodeName.C_Str();
			
			auto it = mBoneMapping.find(boneName);
			if (it == mBoneMapping.end()) continue; // This channel affects a node that isn't a bone
			int boneID = it->second;
			
			// Create a unified set of keyframe timestamps from all tracks
			std::set<double> timestamps;
			for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) timestamps.insert(channel->mPositionKeys[k].mTime);
			for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) timestamps.insert(channel->mRotationKeys[k].mTime);
			for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) timestamps.insert(channel->mScalingKeys[k].mTime);
			
			std::vector<Animation::KeyFrame> keyframes;
			keyframes.reserve(timestamps.size());
			
			// For each unique timestamp, create a keyframe by interpolating values
			for (double time : timestamps) {
				Animation::KeyFrame frame;
				frame.time = time / ticksPerSecond;
				frame.translation = InterpolatePosition(time, channel);
				frame.rotation = InterpolateRotation(time, channel);
				frame.scale = InterpolateScale(time, channel);
				keyframes.push_back(frame);
			}
			
			if (!keyframes.empty()) {
				animation->add_bone_keyframes(boneID, keyframes);
			}
		}
		
		if (!animation->empty()) {
			mAnimations.push_back(std::move(animation));
		}
	}
}

// --- Animation Interpolation Helpers ---

glm::vec3 ModelImporter::InterpolatePosition(double time, const aiNodeAnim* channel) {
	if (channel->mNumPositionKeys == 1) {
		return AssimpToGlmVec3(channel->mPositionKeys[0].mValue);
	}
	
	unsigned int keyIndex = 0;
	for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; i++) {
		if (time < channel->mPositionKeys[i + 1].mTime) {
			keyIndex = i;
			break;
		}
	}
	unsigned int nextKeyIndex = keyIndex + 1;
	if (nextKeyIndex >= channel->mNumPositionKeys) {
		return AssimpToGlmVec3(channel->mPositionKeys[keyIndex].mValue);
	}
	
	double keyTime = channel->mPositionKeys[keyIndex].mTime;
	double nextKeyTime = channel->mPositionKeys[nextKeyIndex].mTime;
	float factor = (time - keyTime) / (nextKeyTime - keyTime);
	
	const aiVector3D& start = channel->mPositionKeys[keyIndex].mValue;
	const aiVector3D& end = channel->mPositionKeys[nextKeyIndex].mValue;
	
	return AssimpToGlmVec3(start + (end - start) * factor);
}

glm::quat ModelImporter::InterpolateRotation(double time, const aiNodeAnim* channel) {
	if (channel->mNumRotationKeys == 1) {
		return AssimpToGlmQuat(channel->mRotationKeys[0].mValue);
	}
	
	unsigned int keyIndex = 0;
	for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; i++) {
		if (time < channel->mRotationKeys[i + 1].mTime) {
			keyIndex = i;
			break;
		}
	}
	unsigned int nextKeyIndex = keyIndex + 1;
	if (nextKeyIndex >= channel->mNumRotationKeys) {
		return AssimpToGlmQuat(channel->mRotationKeys[keyIndex].mValue);
	}
	
	double keyTime = channel->mRotationKeys[keyIndex].mTime;
	double nextKeyTime = channel->mRotationKeys[nextKeyIndex].mTime;
	float factor = (time - keyTime) / (nextKeyTime - keyTime);
	
	aiQuaternion start = channel->mRotationKeys[keyIndex].mValue;
	aiQuaternion end = channel->mRotationKeys[nextKeyIndex].mValue;
	
	aiQuaternion result;
	aiQuaternion::Interpolate(result, start, end, factor);
	
	return AssimpToGlmQuat(result.Normalize());
}

glm::vec3 ModelImporter::InterpolateScale(double time, const aiNodeAnim* channel) {
	if (channel->mNumScalingKeys == 1) {
		return AssimpToGlmVec3(channel->mScalingKeys[0].mValue);
	}
	
	unsigned int keyIndex = 0;
	for (unsigned int i = 0; i < channel->mNumScalingKeys - 1; i++) {
		if (time < channel->mScalingKeys[i + 1].mTime) {
			keyIndex = i;
			break;
		}
	}
	unsigned int nextKeyIndex = keyIndex + 1;
	if (nextKeyIndex >= channel->mNumScalingKeys) {
		return AssimpToGlmVec3(channel->mScalingKeys[keyIndex].mValue);
	}
	
	double keyTime = channel->mScalingKeys[keyIndex].mTime;
	double nextKeyTime = channel->mScalingKeys[nextKeyIndex].mTime;
	float factor = (time - keyTime) / (nextKeyTime - keyTime);
	
	const aiVector3D& start = channel->mScalingKeys[keyIndex].mValue;
	const aiVector3D& end = channel->mScalingKeys[nextKeyIndex].mValue;
	
	return AssimpToGlmVec3(start + (end - start) * factor);
}


// --- Material and Texture Processing ---

void ModelImporter::ProcessMaterials(const aiScene* scene) {
	// This function appears mostly correct, assuming MaterialProperties is defined appropriately.
	// The implementation is left similar to the original.
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
			matPtr->mTextureDiffuse = LoadMaterialTexture(material, scene, aiTextureType_DIFFUSE);
			matPtr->mHasDiffuseTexture = (matPtr->mTextureDiffuse != nullptr);
		}
		// You would add similar blocks for other texture types (specular, normal, etc.)
		
		mMaterialProperties[i] = matPtr;
	}
}

std::shared_ptr<nanogui::Texture> ModelImporter::LoadMaterialTexture(aiMaterial* mat, const aiScene* scene, aiTextureType type) {
	// This function's logic is highly dependent on your specific texture loader.
	// The path resolution logic is kept from the original.
	if (mat->GetTextureCount(type) == 0) {
		return nullptr;
	}
	
	aiString str;
	mat->GetTexture(type, 0, &str);
	std::filesystem::path texturePath(str.C_Str());
	
	// If the path is relative, try to resolve it against the model's directory.
	if (texturePath.is_relative() && !mDirectory.empty()) {
		texturePath = std::filesystem::path(mDirectory) / texturePath;
	}
	
	if (std::filesystem::exists(texturePath)) {
		std::cout << "Found texture: " << texturePath.string() << std::endl;
		//
		// Your actual texture loading implementation would go here.
		// For example: return my_texture_loader.load(texturePath.string());
		//
	} else {
		std::cerr << "Warning: Could not find texture file: " << str.C_Str()
		<< " | Resolved to: " << texturePath.string() << std::endl;
	}
	
	return nullptr; // Placeholder
}


// --- Accessors ---
std::vector<std::unique_ptr<MeshData>>& ModelImporter::GetMeshData() { return mMeshes; }
std::unique_ptr<Skeleton>& ModelImporter::GetSkeleton() { return mSkeleton; }
std::vector<std::unique_ptr<Animation>>& ModelImporter::GetAnimations() { return mAnimations; }
