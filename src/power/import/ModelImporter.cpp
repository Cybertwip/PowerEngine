#include "import/ModelImporter.hpp"

// Your project's data structures
#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>
#include <filesystem>

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
        aiProcess_FlipUVs | // Often needed for OpenGL
        aiProcess_GlobalScale
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
    ProcessMaterials(scene);
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
        aiProcess_FlipUVs |
        aiProcess_GlobalScale,
        formatHint.c_str()
    );

     if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }

    ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
    ProcessMaterials(scene);
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

					skinnedVertex->set_bone(boneID, weight);
                }
            }
        }
        // Normalize weights if needed (your SkinnedMeshVertex should handle this)
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
    // This is a placeholder for your texture loading logic
    // You will need to implement this part based on how your NanoGUI/OpenGL texture loading works.
    aiString str;
    mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
    std::filesystem::path texturePath(str.C_Str());
    
    if (!std::filesystem::exists(texturePath) && !mDirectory.empty()) {
        texturePath = std::filesystem::path(mDirectory) / texturePath.filename();
    }
    
    if (std::filesystem::exists(texturePath)) {
        // Example:
        // std::ifstream file(texturePath, std::ios::binary);
        // std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
        // return std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
        std::cout << "Found texture: " << texturePath << std::endl;
    } else {
        std::cerr << "Warning: Could not find texture file: " << str.C_Str() << std::endl;
    }

    return nullptr;
}


void ModelImporter::BuildSkeleton(const aiScene* scene) {
    mSkeleton = std::make_unique<Skeleton>();
    std::map<std::string, glm::mat4> bindPoses;
    
    // First, find the bind pose for each bone node
    for(auto const& [boneName, boneID] : mBoneMapping) {
        aiNode* boneNode = scene->mRootNode->FindNode(boneName.c_str());
        if (boneNode) {
            bindPoses[boneName] = AssimpToGlmMat4(boneNode->mTransformation);
        }
    }

    // Find the offset matrices in the meshes
    std::map<std::string, glm::mat4> offsetMatrices;
    for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        for(unsigned int b = 0; b < mesh->mNumBones; ++b) {
            aiBone* bone = mesh->mBones[b];
            offsetMatrices[bone->mName.C_Str()] = AssimpToGlmMat4(bone->mOffsetMatrix);
        }
    }

    // Now, build the skeleton hierarchy
    for(auto const& [boneName, boneID] : mBoneMapping) {
        aiNode* node = scene->mRootNode->FindNode(boneName.c_str());
        int parentID = -1;
        if(node && node->mParent) {
            auto it = mBoneMapping.find(node->mParent->mName.C_Str());
            if(it != mBoneMapping.end()) {
                parentID = it->second;
            }
        }
        mSkeleton->add_bone(boneName, offsetMatrices[boneName], bindPoses[boneName], parentID);
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
