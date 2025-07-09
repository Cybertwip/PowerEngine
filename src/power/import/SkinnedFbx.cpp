#include "import/SkinnedFbx.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"
#include "graphics/shading/MeshData.hpp" // Assuming this is the correct header
#include "graphics/shading/MeshVertex.hpp" // Assuming this is the correct header

#include <fbxsdk.h> // Include the main FBX SDK header

#include <algorithm>
#include <execution>
#include <filesystem>
#include <map>
#include <numeric>
#include <thread>
#include <tuple>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace {
// Constants
const int MAX_BONES = 128;
const int MAX_BONE_INFLUENCE = 4;
const int DEFAULT_BONE_ID = 0;
} // namespace

namespace SkinnedFbxUtil {

// Decomposes a matrix into its translation, rotation, and scale components.
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform) {
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, rotation, scale};
}

// Converts an FbxAMatrix to a glm::mat4.
glm::mat4 FbxMatrixToGlm(const FbxAMatrix& fbxMatrix) {
	glm::mat4 result;
	// FBX matrices are column-major, same as GLM. Direct memory copy is safe and fast.
	memcpy(glm::value_ptr(result), (const double*)fbxMatrix, sizeof(glm::mat4));
	return result;
}

} // namespace SkinnedFbxUtil

void SkinnedFbx::ProcessBoneAndParents(FbxNode* boneNode, const std::unordered_map<std::string, FbxAMatrix>& boneOffsetMatrices) {
	if (!boneNode) {
		return;
	}
	
	// Recursively process parent bone first to ensure parent IDs are created before children.
	FbxNode* parentNode = boneNode->GetParent();
	if (parentNode && parentNode->GetSkeleton()) { // Only process parents that are also bones
		ProcessBoneAndParents(parentNode, boneOffsetMatrices);
	}
	
	// Now process the current bone if it hasn't been processed yet.
	std::string boneName = boneNode->GetName();
	if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
		if (mBoneMapping.size() >= MAX_BONES) {
			std::cerr << "Warning: Exceeded maximum number of bones (" << MAX_BONES << "). Bone '" << boneName << "' will be ignored." << std::endl;
			return;
		}
		
		int newBoneID = static_cast<int>(mBoneMapping.size());
		mBoneMapping[boneName] = newBoneID;
		mBoneNodes[boneName] = boneNode;
		
		// The offset matrix (inverse bind pose) is pre-calculated and passed in.
		glm::mat4 offsetMatrix(1.0f); // Default to identity
		auto it = boneOffsetMatrices.find(boneName);
		if (it != boneOffsetMatrices.end()) {
			offsetMatrix = SkinnedFbxUtil::FbxMatrixToGlm(it->second);
		}
		
		// The bind pose is the bone's local transform at the time of binding.
		FbxAMatrix bindPoseMatrix = boneNode->EvaluateLocalTransform();
		glm::mat4 poseMatrix = SkinnedFbxUtil::FbxMatrixToGlm(bindPoseMatrix);
		
		// Store bone info
		BoneHierarchyInfo boneInfo;
		boneInfo.bone_id = newBoneID;
		boneInfo.offset = offsetMatrix;
		boneInfo.bindpose = poseMatrix;
		boneInfo.node = boneNode;
		
		mBoneHierarchy.push_back(boneInfo);
	}
}

void SkinnedFbx::ProcessBones(FbxMesh* mesh) {
	if (!mesh) return;
	
	int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (skinCount == 0) {
		// This mesh is not skinned, so we just call the base implementation (which is empty)
		// and let it be processed as a static mesh.
		Fbx::ProcessBones(mesh);
		return;
	}
	
	if (skinCount > 1) {
		std::cerr << "Warning: More than one skin deformer found on mesh. Only the first will be used." << std::endl;
	}
	
	FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
	if (!skin) return;
	
	if (GetMeshData().empty()) {
		std::cerr << "Error: No mesh data available to process bones." << std::endl;
		return;
	}
	auto& lastProcessedMesh = GetMeshData().back();
	
	// Create a SkinnedMeshData instance to hold vertex bone data.
	// This assumes SkinnedMeshData can be constructed from MeshData.
	mSkinnedMeshes.push_back(std::make_unique<SkinnedMeshData>(*lastProcessedMesh));
	auto& skinnedVertices = mSkinnedMeshes.back()->get_vertices();
	
	// Pre-calculate all bone offset matrices (inverse bind poses).
	std::unordered_map<std::string, FbxAMatrix> boneOffsetMatrices;
	int clusterCount = skin->GetClusterCount();
	for (int i = 0; i < clusterCount; ++i) {
		FbxCluster* cluster = skin->GetCluster(i);
		FbxNode* boneNode = cluster->GetLink();
		if (!boneNode) continue;
		
		std::string boneName = boneNode->GetName();
		FbxAMatrix transformMatrix, transformLinkMatrix;
		cluster->GetTransformMatrix(transformMatrix);       // The mesh's transform at bind time.
		cluster->GetTransformLinkMatrix(transformLinkMatrix); // The bone's global transform at bind time.
		boneOffsetMatrices[boneName] = transformLinkMatrix.Inverse() * transformMatrix;
	}
	
	// Process all bones and their weights.
	for (int i = 0; i < clusterCount; ++i) {
		FbxCluster* cluster = skin->GetCluster(i);
		FbxNode* boneNode = cluster->GetLink();
		if (!boneNode) continue;
		
		// Ensure the bone and its parents are in our hierarchy map.
		ProcessBoneAndParents(boneNode, boneOffsetMatrices);
		
		std::string boneName = boneNode->GetName();
		if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
			continue; // Skip if bone exceeded MAX_BONES limit.
		}
		int boneID = mBoneMapping[boneName];
		
		// Assign weights to vertices.
		int* controlPointIndices = cluster->GetControlPointIndices();
		double* weights = cluster->GetControlPointWeights();
		int indexCount = cluster->GetControlPointIndicesCount();
		
		for (int j = 0; j < indexCount; ++j) {
			int vertexID = controlPointIndices[j];
			float weight = static_cast<float>(weights[j]);
			
			// Add the bone influence to the corresponding vertex.
			// Note: The base Fbx::ProcessMesh creates a unique vertex for each polygon corner.
			// The control point index from the cluster maps to the original vertex position.
			// A robust implementation needs a map from control point index to the final vertex indices.
			// For now, we assume a direct mapping, which may not be correct for complex meshes.
			if (vertexID >= 0 && static_cast<size_t>(vertexID) < skinnedVertices.size()) {
				auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[vertexID]);
				vertex.set_bone(boneID, weight);
			}
		}
	}
	
	// Normalize weights for all vertices.
	for (auto& vertexPtr : skinnedVertices) {
		auto& vertex = static_cast<SkinnedMeshVertex&>(*vertexPtr);
		vertex.normalize_weights();
		if (vertex.has_no_bones()) {
			vertex.set_bone(DEFAULT_BONE_ID, 1.0f);
		}
	}
	
	// Build the skeleton from the processed hierarchy.
	auto skeletonPointer = std::make_unique<Skeleton>();
	for (const auto& boneInfo : mBoneHierarchy) {
		std::string boneName = GetBoneNameById(boneInfo.bone_id);
		int parentIndex = -1;
		FbxNode* parentNode = boneInfo.node->GetParent();
		if (parentNode && parentNode->GetSkeleton()) {
			parentIndex = GetBoneIdByName(parentNode->GetName());
		}
		skeletonPointer->add_bone(boneName, boneInfo.offset, boneInfo.bindpose, parentIndex);
	}
	
	mSkeleton = (skeletonPointer->num_bones() > 0) ? std::move(skeletonPointer) : nullptr;
}

std::string SkinnedFbx::GetBoneNameById(int boneId) const {
	for (const auto& [name, id] : mBoneMapping) {
		if (id == boneId) {
			return name;
		}
	}
	return "";
}

int SkinnedFbx::GetBoneIdByName(const std::string& boneName) const {
	auto it = mBoneMapping.find(boneName);
	if (it != mBoneMapping.end()) {
		return it->second;
	}
	return -1;
}

void SkinnedFbx::TryImportAnimations() {
	// [FIX] Use mScene from the base class instead of the non-existent mSceneLoader.
	if (!mScene) {
		std::cerr << "Error: FBX scene is not loaded, cannot import animations." << std::endl;
		return;
	}
	
	// [FIX] Use mScene directly.
	FbxScene* scene = mScene;
	
	FbxTime::EMode timeMode = scene->GetGlobalSettings().GetTimeMode();
	double frameRate = FbxTime::GetFrameRate(timeMode);
	
	int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < animStackCount; ++i) {
		FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(i);
		scene->SetCurrentAnimationStack(animStack);
		
		int animLayerCount = animStack->GetMemberCount<FbxAnimLayer>();
		for (int j = 0; j < animLayerCount; ++j) {
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(j);
			auto animationPtr = std::make_unique<Animation>();
			Animation& animation = *animationPtr;
			
			float minTime = std::numeric_limits<float>::max();
			float maxTime = std::numeric_limits<float>::lowest();
			
			for (const auto& [boneName, boneIndex] : mBoneMapping) {
				FbxNode* boneNode = mBoneNodes[boneName];
				if (!boneNode) continue;
				
				FbxTimeSpan timeSpan;
				if (!boneNode->GetAnimationInterval(timeSpan, animStack)) {
					continue; // No animation on this bone for this stack
				}
				
				FbxTime startTime = timeSpan.GetStart();
				FbxTime endTime = timeSpan.GetStop();
				FbxTime duration = endTime - startTime;
				
				if (duration.GetSecondDouble() <= 0) continue;
				
				std::vector<Animation::KeyFrame> keyframes;
				for (FbxTime currTime = startTime; currTime <= endTime; currTime += FbxTime::GetOneFrameValue(timeMode)) {
					FbxAMatrix localTransform = boneNode->EvaluateLocalTransform(currTime);
					glm::mat4 transformation = SkinnedFbxUtil::FbxMatrixToGlm(localTransform);
					
					glm::vec3 translation, scale;
					glm::quat rotation;
					std::tie(translation, rotation, scale) = SkinnedFbxUtil::DecomposeTransform(transformation);
					
					Animation::KeyFrame keyframe;
					keyframe.time = static_cast<float>(currTime.GetSecondDouble());
					keyframe.translation = translation;
					keyframe.rotation = rotation;
					keyframe.scale = scale;
					
					keyframes.push_back(keyframe);
					
					minTime = std::min(minTime, keyframe.time);
					maxTime = std::max(maxTime, keyframe.time);
				}
				
				if (!keyframes.empty()) {
					animation.add_bone_keyframes(boneIndex, keyframes);
				}
			}
			
			if (maxTime > minTime) {
				animation.set_duration(maxTime - minTime);
			}
			
			if (!animation.empty()) {
				animation.sort();
				mAnimations.push_back(std::move(animationPtr));
			}
		}
	}
}
