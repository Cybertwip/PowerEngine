#include "import/SkinnedGr2.hpp"
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>

// --- Utility Namespace ---
namespace Gr2Util {
// (Include your GrannyMatrixToGlm and LogCallback helpers here)

// Helper to patch the vertex layout with names. Used by both base and derived classes.
std::vector<granny_data_type_definition> PatchLayout(const granny_vertex_data& vertexData) {
	int componentCount = 0;
	if (vertexData.VertexType) {
		for (const auto* def = vertexData.VertexType; def->Type != GrannyEndMember; ++def) {
			componentCount++;
		}
	}
	
	std::vector<granny_data_type_definition> patchedLayout;
	if (componentCount == 0) return patchedLayout;
	
	patchedLayout.reserve(componentCount + 1);
	for (int i = 0; i < componentCount; ++i) {
		auto newDef = vertexData.VertexType[i];
		newDef.Name = vertexData.VertexComponentNames[i];
		patchedLayout.push_back(newDef);
	}
	patchedLayout.push_back({GrannyEndMember});
	return patchedLayout;
}
}


// --- SkinnedGr2 Class Implementation ---


// This single override efficiently handles both skinned and rigid meshes.
void SkinnedGr2::ProcessMesh(const granny_mesh* mesh, const granny_model* model, int materialBaseIndex) {
	if (!mesh || !mesh->PrimaryVertexData) return;
	
	const auto& vertexData = *mesh->PrimaryVertexData;
	if (vertexData.VertexCount == 0) return;
	
	auto patchedLayout = Gr2Util::PatchLayout(vertexData);
	if (patchedLayout.empty()) {
		std::cerr << "Error: Source vertex layout has no components." << std::endl;
		return;
	}
	
	// A mesh can be part of a skeleton but not have skinning data (rigid).
	if (GrannyMeshIsRigid(mesh)) {
		// Fall back to the base class implementation for rigid meshes.
		Gr2::ProcessMesh(mesh, model, materialBaseIndex);
		return;
	}
	
	// --- Skinned Mesh Processing Path ---
	if (model->Skeleton && !mSkeleton) {
		BuildSkeleton(model->Skeleton);
	}
	
	auto skinnedMesh = std::make_unique<SkinnedMeshData>();
	auto& vertices = skinnedMesh->get_vertices();
	auto& indices = skinnedMesh->get_indices();
	vertices.reserve(vertexData.VertexCount);
	
	// Convert to a vertex type that includes skinning data
	std::vector<granny_pwngt34332_vertex> tempSkinnedVerts(vertexData.VertexCount);
	GrannyConvertVertexLayouts(vertexData.VertexCount, patchedLayout.data(), vertexData.Vertices, GrannyPWNGT34332VertexType, tempSkinnedVerts.data());
	
	// Process all vertex attributes in a single pass
	for (const auto& src : tempSkinnedVerts) {
		auto vertex = std::make_unique<SkinnedMeshVertex>();
		vertex->set_position({src.Position[0], src.Position[1], src.Position[2]});
		vertex->set_normal({src.Normal[0], src.Normal[1], src.Normal[2]});
		vertex->set_texture_coords1({src.UV[0], src.UV[1]});
		
		// Extract bone weights and indices
		for (int j = 0; j < 4; ++j) {
			if (src.BoneWeights[j] > 0) {
				float weight = static_cast<float>(src.BoneWeights[j]) / 255.0f;
				vertex->set_bone(src.BoneIndices[j], weight);
			}
		}
		// vertex->normalize_bones(); // Optional: ensure weights sum to 1.0
		vertices.push_back(std::move(vertex));
	}
	
	// --- Index and Material Processing (copied from original Gr2::ProcessMesh) ---
	const auto& topology = *mesh->PrimaryTopology;
	if (topology.IndexCount > 0) {
		indices.resize(topology.IndexCount);
		GrannyConvertIndices(topology.IndexCount, sizeof(granny_int32), topology.Indices, sizeof(uint32_t), indices.data());
	} else if (topology.Index16Count > 0) {
		// handle 16-bit indices
	}
	
	for (int i = 0; i < topology.GroupCount; ++i) {
		const auto& group = topology.Groups[i];
		for (int tri = 0; tri < group.TriCount; ++tri) {
			// ... (material assignment logic) ...
		}
	}
	
	mSkinnedMeshes.push_back(std::move(skinnedMesh));
}

void SkinnedGr2::BuildSkeleton(const granny_skeleton* skel) {
	mSkeleton = std::make_unique<Skeleton>();
	mBoneMapping.clear();
	
	for (int i = 0; i < skel->BoneCount; ++i) {
		const auto& bone = skel->Bones[i];
		
		glm::mat4 offsetMatrix = Gr2Util::GrannyMatrixToGlm(bone.InverseWorld4x4);
		
		// Decompose local transform for the bind pose matrix
		glm::vec3 pos(bone.LocalTransform.Position[0], bone.LocalTransform.Position[1], bone.LocalTransform.Position[2]);
		glm::quat rot(bone.LocalTransform.Orientation[3], bone.LocalTransform.Orientation[0], bone.LocalTransform.Orientation[1], bone.LocalTransform.Orientation[2]);
		glm::mat4 scaleShear = Gr2Util::GrannyMatrixToGlm(reinterpret_cast<const granny_matrix_4x4&>(bone.LocalTransform.ScaleShear));
		
		glm::mat4 bindPose = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * scaleShear;
		
		mSkeleton->add_bone(bone.Name, offsetMatrix, bindPose, bone.ParentIndex);
		mBoneMapping[bone.Name] = i;
	}
}

void SkinnedGr2::ProcessAnimation(const granny_animation* grnAnim) {
	if (!grnAnim || grnAnim->TrackGroupCount == 0) return;
	
	auto animationPtr = std::make_unique<Animation>();
	Animation& animation = *animationPtr;
	animation.set_duration(grnAnim->Duration);
	
	for (int groupIdx = 0; groupIdx < grnAnim->TrackGroupCount; ++groupIdx) {
		granny_track_group* trackGroup = grnAnim->TrackGroups[groupIdx];
		
		for (int trackIdx = 0; trackIdx < trackGroup->TransformTrackCount; ++trackIdx) {
			granny_transform_track& transformTrack = trackGroup->TransformTracks[trackIdx];
			if (mBoneMapping.find(transformTrack.Name) == mBoneMapping.end()) continue;
			
			int boneIndex = mBoneMapping[transformTrack.Name];
			
			// --- Process Position, Orientation, and Scale/Shear curves ---
			granny_curve2* curves[] = {&transformTrack.PositionCurve, &transformTrack.OrientationCurve, &transformTrack.ScaleShearCurve};
			std::vector<Animation::KeyFrame> keyframes;
			
			// For simplicity, we assume all curves on a track have the same knots.
			// A more robust system would handle them separately.
			granny_curve2& posCurve = transformTrack.PositionCurve;
			if (GrannyCurveIsKeyframed(&posCurve))
			{
				granny_int32x knotCount = GrannyCurveGetKnotCount(&posCurve);
				keyframes.resize(knotCount);
				
				std::vector<granny_real32> knotTimes(knotCount);
				std::vector<granny_real32> posControls(knotCount * 3);
				std::vector<granny_real32> rotControls(knotCount * 4);
				std::vector<granny_real32> ssControls(knotCount * 9);
				
				GrannyCurveExtractKnotValues(&posCurve, 0, knotCount, knotTimes.data(), posControls.data(), GrannyCurveIdentityPosition);
				GrannyCurveExtractKnotValues(&transformTrack.OrientationCurve, 0, knotCount, nullptr, rotControls.data(), GrannyCurveIdentityOrientation);
				GrannyCurveExtractKnotValues(&transformTrack.ScaleShearCurve, 0, knotCount, nullptr, ssControls.data(), GrannyCurveIdentityScaleShear);
				
				for (int k = 0; k < knotCount; ++k) {
					keyframes[k].time = knotTimes[k];
					keyframes[k].translation = glm::make_vec3(&posControls[k * 3]);
					keyframes[k].rotation = glm::quat(rotControls[k*4+3], rotControls[k*4+0], rotControls[k*4+1], rotControls[k*4+2]); // W,X,Y,Z
					// Scale is on the diagonal of the scale-shear matrix
					keyframes[k].scale = { ssControls[k*9 + 0], ssControls[k*9 + 4], ssControls[k*9 + 8] };
				}
			}
			
			if (!keyframes.empty()) {
				animation.add_bone_keyframes(boneIndex, keyframes);
			}
		}
	}
	
	if (!animation.empty()) {
		animation.sort();
		mAnimations.push_back(std::move(animationPtr));
	}
}

void SkinnedGr2::TryImportAnimations() {
	// This requires re-reading the file to get the info struct.
	// In a real app, you'd likely hold onto the granny_file or granny_file_info.
	granny_file* file = GrannyReadEntireFile(mPath.c_str());
	if (!file) return;
	
	granny_file_info* info = GrannyGetFileInfo(file);
	if (info) {
		for (int i = 0; i < info->AnimationCount; ++i) {
			ProcessAnimation(info->Animations[i]);
		}
	}
	
	GrannyFreeFile(file);
}

// Accessor Implementations
const std::unique_ptr<Skeleton>& SkinnedGr2::GetSkeleton() { return mSkeleton; }
void SkinnedGr2::SetSkeleton(std::unique_ptr<Skeleton> skeleton) { mSkeleton = std::move(skeleton); }
std::vector<std::unique_ptr<SkinnedMeshData>>& SkinnedGr2::GetSkinnedMeshData() { return mSkinnedMeshes; }
void SkinnedGr2::SetSkinnedMeshData(std::vector<std::unique_ptr<SkinnedMeshData>>&& meshData) { mSkinnedMeshes = std::move(meshData); }
std::vector<std::unique_ptr<Animation>>& SkinnedGr2::GetAnimationData() { return mAnimations; }
