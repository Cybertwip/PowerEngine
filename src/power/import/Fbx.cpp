#include "Fbx.hpp"

#include <glm/gtc/quaternion.hpp>

#include <filesystem>

namespace {

ozz::math::Transform ToOzzTransform(const sfbx::float4x4& mat) {
    // Extract translation
    ozz::math::Float3 translation(mat[3].x, mat[3].y, mat[3].z);

    // Extract scale
    ozz::math::Float3 scale(
        glm::length(glm::vec3(mat[0].x, mat[0].y, mat[0].z)),
        glm::length(glm::vec3(mat[1].x, mat[1].y, mat[1].z)),
        glm::length(glm::vec3(mat[2].x, mat[2].y, mat[2].z))
    );

    // Extract rotation
    glm::mat3 rotationMatrix(
        glm::vec3(mat[0].x, mat[0].y, mat[0].z) / scale.x,
        glm::vec3(mat[1].x, mat[1].y, mat[1].z) / scale.y,
        glm::vec3(mat[2].x, mat[2].y, mat[2].z) / scale.z
    );

    glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
    ozz::math::Quaternion rotation(rotationQuat.w, rotationQuat.x, rotationQuat.y, rotationQuat.z);

    ozz::math::Transform transform = { translation, rotation, scale };
    return transform;
}

}

Fbx::Fbx(const std::string_view path) {
    LoadModel(path);
}

void Fbx::LoadModel(const std::string_view path) {
    sfbx::DocumentPtr doc = sfbx::MakeDocument(std::string(path));
    if (doc && doc->valid()) {
        ProcessNode(doc->getRootModel());

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
}

void Fbx::ProcessNode(const std::shared_ptr<sfbx::Model> node) {
    if (!node) return;

    if (auto mesh = std::dynamic_pointer_cast<sfbx::Mesh>(node); mesh) {
        ProcessMesh(mesh);
    }

    for (const auto& child : node->getChildren()) {
        if (auto childModel = sfbx::as<sfbx::Model>(child); childModel) {
            ProcessNode(childModel);
        }
    }
}

void Fbx::ProcessMesh(const std::shared_ptr<sfbx::Mesh> mesh) {
    Mesh resultMesh;

    auto geometry = mesh->getGeometry();
    auto points = geometry->getPointsDeformed();
    auto normals = geometry->getNormals();
    auto vertexIndices = geometry->getIndices();

    for (unsigned int i = 0; i < points.size(); ++i) {
        Vertex vertex;
        vertex.mPosition = { points[i].x, points[i].y, points[i].z };
        if (!normals.empty()) {
            vertex.mNormal = { normals[i].x, normals[i].y, normals[i].z };
        }
        resultMesh.mVertices.push_back(vertex);
    }

    auto uvLayers = geometry->getUVLayers();
    int layerIndex = 0;
    for (const auto& uvLayer : uvLayers) {
        for (int i = 0; i < vertexIndices.size(); ++i) {
            int uv_index = uvLayer.indices.empty() ? i : uvLayer.indices[i];
            int index = vertexIndices[i];
            if (layerIndex == 0) {
                resultMesh.mVertices[index].mTexCoords1 = { uvLayer.data[uv_index].x, uvLayer.data[uv_index].y };
                resultMesh.mVertices[index].mTexCoords2 = { uvLayer.data[uv_index].x, uvLayer.data[uv_index].y };
            } else {
                resultMesh.mVertices[index].mTexCoords2 = { uvLayer.data[uv_index].x, uvLayer.data[uv_index].y };
            }
        }
        layerIndex++;
    }

    ProcessBones(mesh);

    for (unsigned int i = 0; i < vertexIndices.size(); ++i) {
        resultMesh.mIndices.push_back(vertexIndices[i]);
    }

    if (mesh->getMaterials().size() > 0) {
        auto material = mesh->getMaterials()[0];
        auto color = material->getAmbientColor();
        resultMesh.mMaterial.mAmbient = { color.x, color.y, color.z };
        color = material->getDiffuseColor();
        resultMesh.mMaterial.mDiffuse = { color.x, color.y, color.z };
        color = material->getSpecularColor();
        resultMesh.mMaterial.mSpecular = { color.x, color.y, color.z };
        resultMesh.mMaterial.mShininess = 0.1f;
        resultMesh.mMaterial.mOpacity = material->getOpacity();
        resultMesh.mMaterial.mHasDiffuseTexture = false;
        // Process textures if needed
    }

    mMeshes.push_back(resultMesh);
}

void Fbx::ProcessBones(const std::shared_ptr<sfbx::Mesh> mesh) {
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
            std::string boneName = std::string{ skinCluster->getChild()->getName() };
            if (mBoneMapping.find(boneName) == mBoneMapping.end()) {
                mBoneMapping[boneName] = static_cast<int>(mBoneMapping.size());
                mBoneTransforms.emplace_back(ToOzzTransform(skinCluster->getTransform()));
            }

            std::size_t boneID = mBoneMapping[boneName];
            auto weights = skinCluster->getWeights();
            auto indices = skinCluster->getIndices();

            for (size_t i = 0; i < weights.size(); ++i) {
                int vertexID = indices[i];
                float weight = weights[i];
                for (int j = 0; j < 4; ++j) {
                    if (mMeshes.back().mVertices[vertexID].mWeights[j] == 0.0f) {
                        mMeshes.back().mVertices[vertexID].mBoneIDs[j] = static_cast<int>(boneID);
                        mMeshes.back().mVertices[vertexID].mWeights[j] = weight;
                        break;
                    }
                }
            }
        }
    }
}
