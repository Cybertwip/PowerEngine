#include "import/Fbx.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace {

glm::mat4 GetUpAxisRotation(int up_axis, int up_axis_sign) {
	glm::mat4 rotation = glm::mat4(1.0f); // Identity matrix
	
	if (up_axis == 0) { // X-Up
		if (up_axis_sign == 1) {
			// Rotate from X+ Up to Y+ Up
			rotation = glm::rotate(rotation, glm::radians(90.0f), glm::vec3(0, 0, 1));
		} else {
			// Rotate from X- Up to Y+ Up
			rotation = glm::rotate(rotation, glm::radians(-90.0f), glm::vec3(0, 0, 1));
		}
	} else if (up_axis == 1) { // Y-Up
		if (up_axis_sign == -1) {
			// Rotate around X-axis by 180 degrees
			rotation = glm::rotate(rotation, glm::radians(180.0f), glm::vec3(1, 0, 0));
		}
		// No rotation needed for positive Y-Up
	} else if (up_axis == 2) { // Z-Up
		if (up_axis_sign == 1) {
			// Rotate from Z+ Up to Y+ Up
			rotation = glm::rotate(rotation, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		} else {
			// Rotate from Z- Up to Y+ Up
			rotation = glm::rotate(rotation, glm::radians(90.0f), glm::vec3(1, 0, 0));
		}
	}
	
	return rotation;
}


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

Fbx::Fbx(const std::string_view path) { LoadModel(path); }

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

void Fbx::ProcessNode(const std::shared_ptr<sfbx::Model>& node) {
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

void Fbx::ProcessMesh(const std::shared_ptr<sfbx::Mesh>& mesh) {
	
	auto path = std::filesystem::absolute(mesh->document().global_settings.path).string();

	auto& resultMesh = mMeshes.emplace_back(std::make_unique<SkinnedMesh::MeshData>());
	
	// Precompute transformation matrices
	const glm::mat4 rotationMatrix = GetUpAxisRotation(mesh->document().global_settings.up_axis,
													   mesh->document().global_settings.up_axis_sign);
	const float scaleFactor = static_cast<float>(mesh->document().global_settings.unit_scale);
	const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
	const glm::mat4 transformMatrix = rotationMatrix * scaleMatrix;
	
	auto geometry = mesh->getGeometry();
	const auto& points = geometry->getPointsDeformed();
	const auto& vertexIndices = geometry->getIndices();
	
	// Reserve space for vertices and indices
	const size_t vertexCount = vertexIndices.size();
	resultMesh->mVertices.resize(vertexCount);
	resultMesh->mIndices.resize(vertexCount);
	
	// Retrieve materials and map them to indices
	const auto& materials = mesh->getMaterials();
	std::unordered_map<std::shared_ptr<sfbx::Material>, int64_t> materialIndexMap;
	for (size_t i = 0; i < materials.size(); ++i) {
		materialIndexMap[materials[i]] = static_cast<int64_t>(i);
	}
	
	// Precompute normal indices
	const auto& normalLayers = geometry->getNormalLayers();
	std::vector<int> normalIndices(vertexCount, -1);
	sfbx::RawVector<sfbx::float3> normalData;
	if (!normalLayers.empty()) {
		const auto& normalLayer = normalLayers[0];
		normalData = normalLayer.data;
		const auto mappingMode = normalLayer.mapping_mode;
		const auto referenceMode = normalLayer.reference_mode;
		const auto& normalLayerIndices = normalLayer.indices;
		
		for (size_t i = 0; i < vertexCount; ++i) {
			int controlPointIndex = vertexIndices[i];
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				normalIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : normalLayerIndices[i];
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				normalIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : normalLayerIndices[controlPointIndex];
			}
		}
	}
	
	// Precompute UV indices for each layer
	const auto& uvLayers = geometry->getUVLayers();
	std::vector<std::vector<int>> uvIndicesPerLayer(uvLayers.size(), std::vector<int>(vertexCount, -1));
	for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
		const auto& uvLayer = uvLayers[layerIndex];
		const auto mappingMode = uvLayer.mapping_mode;
		const auto referenceMode = uvLayer.reference_mode;
		const auto& uvLayerIndices = uvLayer.indices;
		
		for (size_t i = 0; i < vertexCount; ++i) {
			int controlPointIndex = vertexIndices[i];
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				uvIndicesPerLayer[layerIndex][i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : uvLayerIndices[i];
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				uvIndicesPerLayer[layerIndex][i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : uvLayerIndices[controlPointIndex];
			}
		}
	}
	
	// Precompute material indices per vertex
	std::vector<int> materialIndices(vertexCount, -1);
	for (size_t i = 0; i < vertexCount; ++i) {
		materialIndices[i] = geometry->getMaterialForVertexIndex(static_cast<int>(i));
	}
	
	// Determine the number of threads to use
	unsigned int numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) numThreads = 1;
	
	// Divide the work among threads
	std::vector<std::thread> threads(numThreads);
	size_t chunkSize = vertexCount / numThreads;
	size_t remainder = vertexCount % numThreads;
	
	auto processVertices = [&](size_t startIndex, size_t endIndex) {
		for (size_t i = startIndex; i < endIndex; ++i) {
			int controlPointIndex = vertexIndices[i];
			SkinnedMesh::Vertex vertex;
			
			// Transform position
			const auto& point = points[controlPointIndex];
			glm::vec4 position(point.x, point.y, point.z, 1.0f);
			position = transformMatrix * position;
			vertex.set_position({ position.x, position.y, position.z });
			
			// Transform normal
			if (!normalData.empty() && normalIndices[i] >= 0) {
				const auto& normal = normalData[normalIndices[i]];
				glm::vec4 normalVec(normal.x, normal.y, normal.z, 0.0f);
				normalVec = rotationMatrix * normalVec;
				normalVec = glm::normalize(normalVec);
				vertex.set_normal({ normalVec.x, normalVec.y, normalVec.z });
			} else {
				vertex.set_normal({ 0.0f, 1.0f, 0.0f });
			}
			
			// Set UVs
			for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
				int uvIndex = uvIndicesPerLayer[layerIndex][i];
				if (uvIndex >= 0) {
					const auto& uv = uvLayers[layerIndex].data[uvIndex];
					if (layerIndex == 0) {
						vertex.set_texture_coords1({ uv.x, uv.y });
						vertex.set_texture_coords2({ uv.x, uv.y });
					} else {
						vertex.set_texture_coords2({ uv.x, uv.y });
					}
				} else {
					if (layerIndex == 0) {
						vertex.set_texture_coords1({ 0.0f, 0.0f });
						vertex.set_texture_coords2({ 0.0f, 0.0f });
					} else {
						vertex.set_texture_coords2({ 0.0f, 0.0f });
					}
				}
			}
			
			// Set material ID
			int matIndex = materialIndices[i];
			vertex.set_texture_id((matIndex >= 0 && matIndex < static_cast<int>(materials.size())) ? matIndex : -1);
			
			// Assign vertex and index
			resultMesh->mVertices[i] = std::move(vertex);
			resultMesh->mIndices[i] = static_cast<uint32_t>(i);
		}
	};
	
	size_t startIndex = 0;
	for (unsigned int t = 0; t < numThreads; ++t) {
		size_t endIndex = startIndex + chunkSize + (t < remainder ? 1 : 0);
		threads[t] = std::thread(processVertices, startIndex, endIndex);
		startIndex = endIndex;
	}
	
	// Wait for all threads to finish
	for (auto& thread : threads) {
		thread.join();
	}
	
	// Process bones (assuming ProcessBones is efficient)
	ProcessBones(mesh);
	
	// Process materials
	resultMesh->mMaterials.reserve(materials.size());
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto& material = materials[i];
		auto matPtr = std::make_shared<MaterialProperties>();
		MaterialProperties& matData = *matPtr;
		
		// Combine the path and index into a single string
		std::string combined = path + std::to_string(i);
		
		// Compute the hash of the combined string
		std::size_t hashValue = std::hash<std::string>{}(combined);
		
		// Assign to mIdentifier
		matData.mIdentifier = hashValue;
		
		// Set material properties
		auto color = material->getAmbientColor();
		matData.mAmbient = glm::vec4{ color.x, color.y, color.z, 1.0f };
		color = material->getDiffuseColor();
		matData.mDiffuse = glm::vec4{ color.x, color.y, color.z, 1.0f };
		color = material->getSpecularColor();
		matData.mSpecular = glm::vec4{ color.x, color.y, color.z, 1.0f};
		matData.mShininess = 0.1f;
		matData.mOpacity = material->getOpacity();
		matData.mHasDiffuseTexture = false;
		
		// Load texture if available
		if (auto fbxTexture = material->getTexture("DiffuseColor"); fbxTexture && !fbxTexture->getData().empty()) {
			matData.mTextureDiffuse = std::make_unique<nanogui::Texture>(
																		 fbxTexture->getData().data(), static_cast<int>(fbxTexture->getData().size()));
			matData.mHasDiffuseTexture = true;
		}
		
		resultMesh->mMaterials.push_back(matPtr);
	}
}

void Fbx::ProcessBones(const std::shared_ptr<sfbx::Mesh>& mesh) {
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
                mBoneTransforms.emplace_back(ToOzzTransform(skinCluster->getTransform()));
            }

            std::size_t boneID = mBoneMapping[boneName];
            auto weights = skinCluster->getWeights();
            auto indices = skinCluster->getIndices();

            for (size_t i = 0; i < weights.size(); ++i) {
                int vertexID = indices[i];
                float weight = weights[i];
                for (int j = 0; j < 4; ++j) {
					if (mMeshes.back()->mVertices[vertexID].get_weights()[j] == 0.0f) {
						mMeshes.back()->mVertices[vertexID].set_bone(static_cast<int>(boneID),
                                                                      weight);
                        break;
                    }
                }
            }
        }
    }
}
