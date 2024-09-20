#include "import/SkinnedFbx.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace SkinnedFbxUtil {
ozz::math::Transform ToOzzTransform(const sfbx::float4x4& mat) {
    // Extract translation
    ozz::math::Float3 translation(mat[3].x, mat[3].y, mat[3].z);

    // Extract scale
    ozz::math::Float3 scale(glm::length(glm::vec3(mat[0].x, mat[0].y, mat[0].z)),
                            glm::length(glm::vec3(mat[1].x, mat[1].y, mat[1].z)),
                            glm::length(glm::vec3(mat[2].x, mat[2].y, mat[2].z)));

    // Extract rotation
    glm::mat3 rotationMatrix(glm::vec3(mat[0].x, mat[0].y, mat[0].z) / scale.x,
                             glm::vec3(mat[1].x, mat[1].y, mat[1].z) / scale.y,
                             glm::vec3(mat[2].x, mat[2].y, mat[2].z) / scale.z);

    glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
    ozz::math::Quaternion rotation(rotationQuat.w, rotationQuat.x, rotationQuat.y, rotationQuat.z);

    ozz::math::Transform transform = {translation, rotation, scale};
    return transform;
}

}  // namespace

SkinnedFbx::SkinnedFbx(const std::string& path) : Fbx(path) {
}

void SkinnedFbx::ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
		
	auto& lastProcessedMesh = GetMeshData().back();
	
	mSkinnedMeshes.push_back(std::make_unique<SkinnedMeshData>(std::move(lastProcessedMesh)));

    auto geometry = mesh->getGeometry();
    auto deformers = geometry->getDeformers();
    std::shared_ptr<sfbx::Skin> skin;

    for (auto& deformer : deformers) {
        if (skin = sfbx::as<sfbx::Skin>(deformer); skin) {
            break;
        }
    }

    if (skin) {
        auto skinClusters = skin->getChildren();
        for (const auto& clusterObj : skinClusters) {
            auto skinCluster = sfbx::as<sfbx::Cluster>(clusterObj);
            std::string boneName = std::string{skinCluster->getChild()->getName()};
            if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
                mBoneMapping[boneName] = static_cast<int>(mBoneMapping.size());
				mBoneTransforms.emplace_back(SkinnedFbxUtil::ToOzzTransform(skinCluster->getTransform()));
            }

            std::size_t boneID = mBoneMapping[boneName];
            auto weights = skinCluster->getWeights();
            auto indices = skinCluster->getIndices();

            for (size_t i = 0; i < weights.size(); ++i) {
                int vertexID = indices[i];
                float weight = weights[i];
                for (int j = 0; j < 4; ++j) {
					if (mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].get_weights()[j] == 0.0f) {
						mSkinnedMeshes.back()->get_skinned_vertices()[vertexID].set_bone(static_cast<int>(boneID),
                                                                      weight);
                        break;
                    }
                }
            }
        }
    }
}

void SkinnedFbx::TryBuildSkeleton() {
	
	// Build the Ozz skeleton
	ozz::animation::offline::RawSkeleton rawSkeleton;
	rawSkeleton.roots.resize(mBoneMapping.size());
	
	int index = 0;
	for (const auto& bone : mBoneMapping) {
		ozz::animation::offline::RawSkeleton::Joint joint;
		joint.name = bone.first.c_str();
		joint.transform = mBoneTransforms[bone.second];
		rawSkeleton.roots[index] = joint;
		index++;
	}
	
	ozz::animation::offline::SkeletonBuilder skeletonBuilder;
	mSkeleton = skeletonBuilder(rawSkeleton);
}
