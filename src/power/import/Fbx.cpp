#include "import/Fbx.hpp"

#include <filesystem>
#include <map>
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
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<SkinnedMesh::MeshData>());
	
	// Get rotation matrix based on UpAxis
	glm::mat4 rotationMatrix = GetUpAxisRotation(mesh->document().global_settings.up_axis, mesh->document().global_settings.up_axis_sign);
	
	// Optionally, apply unit scaling
	float scaleFactor = static_cast<float>(mesh->document().global_settings.unit_scale);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
	
	glm::mat4 transformMatrix = rotationMatrix * scaleMatrix;
	
	auto geometry = mesh->getGeometry();
	auto points = geometry->getPointsDeformed();
	auto vertexIndices = geometry->getIndices();
	
	// Process UV layers
	auto uvLayers = geometry->getUVLayers();
	
	const auto& materials = mesh->getMaterials();
	
	std::map<std::shared_ptr<sfbx::Material>, int64_t> materialIndexMap;  // Map from Material pointer to index
	
	for (int i = 0; i < materials.size(); ++i) {
		materialIndexMap[materials[i]] = i;
	}
	
	std::vector<std::shared_ptr<sfbx::Material>> indexMaterialMap(materials.begin(), materials.end());
	
	// Create the vertices per polygon vertex
	for (unsigned int i = 0; i < vertexIndices.size(); ++i) {
		int controlPointIndex = vertexIndices[i];
		auto vertex = std::make_unique<SkinnedMesh::Vertex>();
		
		// Set position
		const auto& point = points[controlPointIndex];
		glm::vec4 position(point.x, point.y, point.z, 1.0f);
		
		// Apply transformation (rotation and scaling)
		position = transformMatrix * position;
		vertex->set_position({position.x, position.y, position.z});
		
		// Process normals
		const auto& normalLayers = geometry->getNormalLayers();
		if (!normalLayers.empty()) {
			// For simplicity, we'll process the first normal layer
			const auto& normalLayer = normalLayers[0];
			const sfbx::LayerMappingMode mappingMode = normalLayer.mapping_mode;
			const sfbx::LayerReferenceMode referenceMode = normalLayer.reference_mode;
			
			int normalIndex = -1;
			
			// Determine normalIndex based on mapping and reference modes
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				if (referenceMode == sfbx::LayerReferenceMode::Direct) {
					normalIndex = i;
				} else if (referenceMode == sfbx::LayerReferenceMode::IndexToDirect) {
					normalIndex = normalLayer.indices[i];
				}
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				if (referenceMode == sfbx::LayerReferenceMode::Direct) {
					normalIndex = controlPointIndex;
				} else if (referenceMode == sfbx::LayerReferenceMode::IndexToDirect) {
					normalIndex = normalLayer.indices[controlPointIndex];
				}
			} else {
				// Handle other mapping modes if necessary
				normalIndex = -1;
			}
			
			if (normalIndex >= 0 && normalIndex < normalLayer.data.size()) {
				const auto& normal = normalLayer.data[normalIndex];
				glm::vec4 normalVec(normal.x, normal.y, normal.z, 0.0f); // w = 0 for normals
				
				// Apply rotation (do not apply scaling to normals)
				normalVec = rotationMatrix * normalVec;
				normalVec = glm::normalize(normalVec);
				vertex->set_normal({normalVec.x, normalVec.y, normalVec.z});
			} else {
				// Set a default normal if index is invalid
				vertex->set_normal({0.0f, 1.0f, 0.0f});
			}
		} else {
			// Set a default normal if no normal layers are available
			vertex->set_normal({0.0f, 1.0f, 0.0f});
		}
		
		// Process UV layers
		int layerIndex = 0;
		for (const auto& uvLayer : uvLayers) {
			const std::string& uvMappingMode = uvLayer.mapping_mode;
			const std::string& uvReferenceMode = uvLayer.reference_mode;
			
			int uvIndex = -1;
			
			// Determine uvIndex based on mapping and reference modes
			if (uvMappingMode == "ByPolygonVertex") {
				if (uvReferenceMode == "Direct") {
					uvIndex = i;
				} else if (uvReferenceMode == "IndexToDirect") {
					uvIndex = uvLayer.indices[i];
				}
			} else if (uvMappingMode == "ByControlPoint") {
				if (uvReferenceMode == "Direct") {
					uvIndex = controlPointIndex;
				} else if (uvReferenceMode == "IndexToDirect") {
					uvIndex = uvLayer.indices[controlPointIndex];
				}
			} else {
				// Handle other mapping modes if necessary
				uvIndex = -1;
			}
			
			if (uvIndex >= 0 && uvIndex < uvLayer.data.size()) {
				float u = uvLayer.data[uvIndex].x;
				float v = uvLayer.data[uvIndex].y;
				if (layerIndex == 0) {
					vertex->set_texture_coords1({u, v});
					vertex->set_texture_coords2({u, v});
				} else {
					vertex->set_texture_coords2({u, v});
				}
			} else {
				// Set default UVs if index is invalid
				if (layerIndex == 0) {
					vertex->set_texture_coords1({0.0f, 0.0f});
					vertex->set_texture_coords2({0.0f, 0.0f});
				} else {
					vertex->set_texture_coords2({0.0f, 0.0f});
				}
			}
			layerIndex++;
		}
		
		// Get material for this vertex
		int materialIndex = geometry->getMaterialForVertexIndex(i); // i is the polygon vertex index
		if (materialIndex >= 0 && materialIndex < indexMaterialMap.size()) {
			auto material = indexMaterialMap[materialIndex];
			int textureId = materialIndexMap[material];
			vertex->set_texture_id(textureId);
		} else {
			vertex->set_texture_id(-1); // No material assigned
		}
		
		resultMesh->mVertices.push_back(std::move(vertex));
		resultMesh->mIndices.push_back(i); // Sequential indices
	}
	
	// Process the bones for skinning
	ProcessBones(mesh);
	
	// Process all materials in the mesh
	for (auto& [material, index] : materialIndexMap) {
		MaterialProperties matData;  // Store material properties for each material
		
		// Process material colors
		auto color = material->getAmbientColor();
		matData.mAmbient = {color.x, color.y, color.z};
		
		color = material->getDiffuseColor();
		matData.mDiffuse = {color.x, color.y, color.z};
		
		color = material->getSpecularColor();
		matData.mSpecular = {color.x, color.y, color.z};
		
		matData.mShininess = 0.1f;
		matData.mOpacity = material->getOpacity();
		matData.mHasDiffuseTexture = false;
		
		// Process textures if available
		if (auto fbxTexture = material->getTexture("DiffuseColor")) {
			if (!fbxTexture->getData().empty()) {
				// Assuming nanogui::Texture is compatible with the loaded image data
				matData.mTextureDiffuse = std::make_unique<nanogui::Texture>(
																			 fbxTexture->getData().data(), static_cast<int>(fbxTexture->getData().size()));
				matData.mHasDiffuseTexture = true;
			}
		}
		
		// Add the processed material to the resultMesh's material collection
		resultMesh->mMaterials.push_back(std::move(matData));
	}
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
                    if (mMeshes.back()->mVertices[vertexID]->get_weights()[j] == 0.0f) {
                        mMeshes.back()->mVertices[vertexID]->set_bone(static_cast<int>(boneID),
                                                                      weight);
                        break;
                    }
                }
            }
        }
    }
}
