#include "import/SkinnedFbx.hpp"

#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace {

// Constants
const int MAX_BONES = 128;
const int MAX_BONE_INFLUENCE = 4;
const int DEFAULT_BONE_ID = 0; // Ensure this bone exists in your shader and skeleton

}

namespace SkinnedFbxUtil {


// return translate, rotate, scale
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4 &transform)
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, rotation, scale};
}
glm::vec3 ExtractScale(const glm::mat4& matrix) {
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));
	return scale;
}

glm::mat4 FbxMatrixToGlm(const FbxAMatrix& fbxMatrix) {
	glm::mat4 result;
	for (int col = 0; col < 4; col++) {
		FbxVector4 column = fbxMatrix.GetColumn(col);
		result[0][col] = static_cast<float>(column[0]);
		result[1][col] = static_cast<float>(column[1]);
		result[2][col] = static_cast<float>(column[2]);
		result[3][col] = static_cast<float>(column[3]);
	}
	return result;
}

}  // namespace

void SkinnedFbx::ProcessBoneAndParents(FbxNode* boneNode, const std::unordered_map<std::string, FbxAMatrix>& boneOffsetMatrices) {
	if (!boneNode) {
		return;
	}
	
	// Recursively process parent bone first
	FbxNode* parentNode = boneNode->GetParent();
	if (parentNode) {
		ProcessBoneAndParents(parentNode, boneOffsetMatrices);
	}
	
	// Now process the current bone
	std::string boneName = boneNode->GetName();
	if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
		if (mBoneMapping.size() >= MAX_BONES) {
			std::cerr << "Error: Exceeded maximum number of bones (" << MAX_BONES << "). Bone " << boneName << " cannot be assigned." << std::endl;
			return; // Skip this bone
		}
		
		int newBoneID = static_cast<int>(mBoneMapping.size());
		mBoneMapping[boneName] = newBoneID;
		mBoneNodes[boneName] = boneNode; // Store bone node
		
		// Get the offset matrix from boneOffsetMatrices, or use identity if not found
		glm::mat4 offsetMatrix;
		auto it = boneOffsetMatrices.find(boneName);
		if (it != boneOffsetMatrices.end()) {
			offsetMatrix = SkinnedFbxUtil::FbxMatrixToGlm(it->second);
		} else {
			// If the bone is not directly associated with a skin cluster, use identity matrix
			offsetMatrix = glm::mat4(1.0f);
		}
		
		// Get local bind pose matrix
		FbxAMatrix bindPoseMatrix = boneNode->EvaluateLocalTransform();
		
		glm::mat4 poseMatrix = SkinnedFbxUtil::FbxMatrixToGlm(bindPoseMatrix);
		
		// Store bone info
		BoneHierarchyInfo boneInfo;
		boneInfo.bone_id = newBoneID;
		boneInfo.offset = offsetMatrix;
		boneInfo.bindpose = poseMatrix;
		boneInfo.node = boneNode;
		
		// Add to hierarchy
		mBoneHierarchy.push_back(boneInfo);
	}
}


void SkinnedFbx::ProcessBones(FbxMesh* mesh) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	// Get the number of skins attached to the mesh
	int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (skinCount == 0) {
		std::cerr << "Warning: No skin deformer found for the mesh." << std::endl;
		return;
	}
	
	if (skinCount > 1) {
		std::cerr << "Warning: More than one skin found, only processing the first one." << std::endl;
	}
	
	// Get the first skin deformer
	FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	
	// Retrieve the last processed mesh data
	if (GetMeshData().empty()) {
		std::cerr << "Error: No mesh data available to process bones." << std::endl;
		return;
	}
	auto& lastProcessedMesh = GetMeshData().back();
	
	// Create a new SkinnedMeshData instance and add it to mSkinnedMeshes
	mSkinnedMeshes.push_back(std::make_unique<SkinnedMeshData>(*lastProcessedMesh));
	
	// Initialize skinned vertices
	auto& skinnedVertices = mSkinnedMeshes.back()->get_vertices();
	
	// Map to store bone offset matrices
	std::unordered_map<std::string, FbxAMatrix> boneOffsetMatrices;
	
	// Process clusters (bones)
	int clusterCount = skin->GetClusterCount();
	for (int i = 0; i < clusterCount; ++i) {
		FbxCluster* cluster = skin->GetCluster(i);
		
		FbxNode* boneNode = cluster->GetLink();
		if (!boneNode) {
			std::cerr << "Error: Cluster has no link." << std::endl;
			continue;
		}
		
		std::string boneName = boneNode->GetName();
		
		// Compute the offset matrix (inverse bind pose)
		FbxAMatrix transformMatrix;
		cluster->GetTransformMatrix(transformMatrix); // Mesh's transform
		FbxAMatrix transformLinkMatrix;
		cluster->GetTransformLinkMatrix(transformLinkMatrix); // Bone's transform
		
		FbxAMatrix inverseBindPose = transformLinkMatrix.Inverse() * transformMatrix;
		
		// Store the offset matrix
		boneOffsetMatrices[boneName] = inverseBindPose;
		
		// Process this bone and its parents, passing the offset matrices map
		ProcessBoneAndParents(boneNode, boneOffsetMatrices);
		
		// Now you can safely get the bone ID
		int boneID = mBoneMapping[boneName];
		
		// Assign weights to vertices
		int* indices = cluster->GetControlPointIndices();
		double* weights = cluster->GetControlPointWeights();
		int indexCount = cluster->GetControlPointIndicesCount();
		
		// Assign actual weights
		for (int j = 0; j < indexCount; ++j) {
			int vertexID = indices[j];
			double weight = weights[j];
			
			// Validate weight
			if (weight <= 0.0) {
				std::cerr << "Warning: Non-positive weight (" << weight << ") for Vertex ID: " << vertexID << " in Bone: " << boneName << std::endl;
				continue;
			}
			
			// Ensure valid vertex ID
			if (vertexID >= 0 && static_cast<size_t>(vertexID) < skinnedVertices.size()) {
				auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[vertexID]);
				vertex.set_bone(boneID, static_cast<float>(weight));
			} else {
				std::cerr << "Error: Invalid vertex ID (" << vertexID << ") for bone: " << boneName << ". Skinned Vertices Size: " << skinnedVertices.size() << std::endl;
			}
		}
	}
	
	// Post-Processing: Assign default bone to vertices with no influences
	for (size_t i = 0; i < skinnedVertices.size(); ++i) {
		auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[i]);
		if (vertex.has_no_bones()) {
			vertex.set_bone(DEFAULT_BONE_ID, 1.0f);
			std::cerr << "Info: Vertex ID " << i << " had no bone influences. Assigned to Default Bone ID " << DEFAULT_BONE_ID << "." << std::endl;
		}
	}
	
	// Validate that all vertices have at least one bone influence
	for (size_t i = 0; i < skinnedVertices.size(); ++i) {
		auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[i]);
		if (vertex.has_no_bones()) {
			std::cerr << "Error: Vertex ID " << i << " still has no bone influences after default assignment." << std::endl;
		}
	}
	
	// Build skeleton
	auto skeletonPointer = std::make_unique<Skeleton>();
	auto& skeleton = *skeletonPointer;
	
	// Process mBoneHierarchy to create skeleton bones
	for (const auto& boneInfo : mBoneHierarchy) {
		std::string boneName = GetBoneNameById(boneInfo.bone_id);
		
		// Determine parent index
		int parentIndex = -1;
		FbxNode* parentBone = boneInfo.node->GetParent();
		if (parentBone) {
			std::string parentBoneName = parentBone->GetName();
			int parentId = GetBoneIdByName(parentBoneName);
			
			if (parentId != -1) {
				parentIndex = parentId;
			} else {
				// Parent bone may not have been processed yet
				// Process the parent bone
				ProcessBoneAndParents(parentBone, boneOffsetMatrices);
				parentId = GetBoneIdByName(parentBoneName);
				if (parentId != -1) {
					parentIndex = parentId;
				} else {
					std::cerr << "Warning: Parent bone not found for bone: " << boneName << std::endl;
				}
			}
		}
		
		// Add bone to skeleton with the correct offset and bind pose matrices
		skeleton.add_bone(boneName, boneInfo.offset, boneInfo.bindpose, parentIndex);
	}
	
	// Assign the skeleton if it has bones
	mSkeleton = skeleton.num_bones() > 0 ? std::move(skeletonPointer) : nullptr;
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
	for (const auto& [name, id] : mBoneMapping) {
		if (name == boneName) {
			return id;
		}
	}
	return -1;
}

void SkinnedFbx::TryImportAnimations() {
	if (!mSceneLoader->scene()) {
		std::cerr << "Error: FBX scene is not loaded." << std::endl;
		return;
	}
	
	FbxScene* scene = mSceneLoader->scene();
	
	// Get the global time mode and frame rate
	FbxTime::EMode timeMode = scene->GetGlobalSettings().GetTimeMode();
	double frameRate = FbxTime::GetFrameRate(timeMode);
	
	int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < animStackCount; ++i) {
		FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(i);
		
		// Set current animation stack
		scene->SetCurrentAnimationStack(animStack);
		
		// Get the animation layers
		int animLayerCount = animStack->GetMemberCount<FbxAnimLayer>();
		for (int j = 0; j < animLayerCount; ++j) {
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(j);
			
			auto animationPtr = std::make_unique<Animation>();
			Animation& animation = *animationPtr;
			
			// Variables to track min and max keyframe times
			float minTime = std::numeric_limits<float>::max();
			float maxTime = std::numeric_limits<float>::lowest();
			
			// Iterate through bone mapping to create keyframes for each bone
			for (const auto& bone : mBoneMapping) {
				const std::string& boneName = bone.first;
				int boneIndex = bone.second;
				
				FbxNode* boneNode = mBoneNodes[boneName];
				if (!boneNode) {
					continue;
				}
				
				// Get the time span of the animation for this bone
				FbxTimeSpan timeSpan;
				boneNode->GetAnimationInterval(timeSpan, animStack);
				
				FbxTime startTime = timeSpan.GetStart();
				FbxTime endTime = timeSpan.GetStop();
				
				// Sample the animation at the specified frame rate
				double startSeconds = startTime.GetSecondDouble();
				double endSeconds = endTime.GetSecondDouble();
				
				double timeStep = 1.0 / frameRate;
				
				std::vector<Animation::KeyFrame> keyframes;
				
				for (double time = startSeconds; time <= endSeconds; time += timeStep) {
					FbxTime currTime;
					currTime.SetSecondDouble(time);
					
					// Evaluate the node's local transform at currTime
					FbxAMatrix localTransform = boneNode->EvaluateLocalTransform(currTime);
					
					// Convert to glm::mat4
					glm::mat4 transformation = SkinnedFbxUtil::FbxMatrixToGlm(localTransform);
					
					transformation = glm::inverse(mBoneHierarchy[boneIndex].bindpose) * transformation;
					
					// Decompose the transformation
					glm::vec3 translation;
					glm::quat rotation;
					glm::vec3 scale;
					std::tie(translation, rotation, scale) = SkinnedFbxUtil::DecomposeTransform(transformation);
					
					// Store the keyframe
					Animation::KeyFrame keyframe;
					keyframe.time = static_cast<float>(time - startSeconds); // Adjust time relative to start
					keyframe.translation = translation;
					keyframe.rotation = rotation;
					keyframe.scale = scale;
					
					keyframes.push_back(keyframe);
					
					// Update min and max times
					minTime = std::min(minTime, keyframe.time);
					maxTime = std::max(maxTime, keyframe.time);
				}
				
				// Add the bone's keyframes to the animation
				if (!keyframes.empty()) {
					animation.add_bone_keyframes(boneIndex, keyframes);
				}
			}
			
			// Set the animation's duration based on keyframe times
			if (minTime < maxTime) {
				animation.set_duration(maxTime - minTime);
			} else {
				animation.set_duration(0.0f);
			}
			
			// Add the animation to the mAnimations vector
			if (!animation.empty()) {
				animation.sort();
				mAnimations.push_back(std::move(animationPtr));
			}
		}
	}
}
