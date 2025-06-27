#include "import/SkinnedGr2.hpp"
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>

// --- SkinnedGr2 Class Implementation ---

void SkinnedGr2::ProcessSkeletons(const granny_model* model, const granny_mesh* mesh, MeshData& meshData) {
	if (!model || !model->Skeleton || !GrannyMeshIsRigid(mesh)) {
		return; // Not a skinned mesh or no skeleton
	}
	
	// Build skeleton once
	if (!mSkeleton) {
		BuildSkeleton(model->Skeleton);
	}
	
	// Create a skinned mesh from the base mesh data
	auto skinnedMesh = std::make_unique<SkinnedMeshData>(meshData);
	auto& skinnedVertices = skinnedMesh->get_vertices();
	
	const auto& vertexData = *mesh->PrimaryVertexData;
	// Use a vertex type that includes weights (up to 4 influences)
	std::vector<granny_pwngt34332_vertex> tempSkinnedVerts(vertexData.VertexCount);
	GrannyConvertVertexLayouts(vertexData.VertexCount, vertexData.VertexType, vertexData.Vertices, GrannyPWNGT34332VertexType, tempSkinnedVerts.data());
	
	for (size_t i = 0; i < skinnedVertices.size(); ++i) {
		auto& vertex = static_cast<SkinnedMeshVertex&>(*skinnedVertices[i]);
		const auto& src = tempSkinnedVerts[i];
		
		for (int j = 0; j < 4; ++j) {
			if (src.BoneWeights[j] > 0) {
				float weight = static_cast<float>(src.BoneWeights[j]) / 255.0f;
				vertex.set_bone(src.BoneIndices[j], weight);
			}
		}
//		vertex.normalize_bones(); // Ensure weights sum to 1.0
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
