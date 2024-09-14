#include "import/Fbx.hpp"

#include <filesystem>
#include <unordered_map>
#include <utility> // for std::pair
#include <glm/gtc/quaternion.hpp>

// Custom hash function for std::pair<int, int>
namespace std {
template <>
struct hash<std::pair<int, int>> {
	std::size_t operator()(const std::pair<int, int>& p) const noexcept {
		// Combine the two integers into a single hash value
		std::size_t h1 = std::hash<int>{}(p.first);
		std::size_t h2 = std::hash<int>{}(p.second);
		return h1 ^ (h2 << 1); // Simple combination
	}
};
}

namespace {

// Precompute rotation matrices for each possible up axis to avoid recalculating them
const std::unordered_map<std::pair<int, int>, glm::mat4, std::hash<std::pair<int, int>>> precomputedRotations = {
	{{0, 1}, glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))},
	{{0, -1}, glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 0, 1))},
	{{1, -1}, glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0))},
	{{2, 1}, glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0))},
	{{2, -1}, glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))},
};

glm::mat4 GetUpAxisRotation(int up_axis, int up_axis_sign) {
	auto key = std::make_pair(up_axis, up_axis_sign);
	auto it = precomputedRotations.find(key);
	if (it != precomputedRotations.end()) {
		return it->second;
	}
	return glm::mat4(1.0f); // Identity matrix if no rotation is needed
}

ozz::math::Transform ToOzzTransform(const sfbx::float4x4& mat) {
	// Extract translation
	ozz::math::Float3 translation(mat[3].x, mat[3].y, mat[3].z);
	
	// Extract scale and normalize rotation matrix
	glm::vec3 scale(
					glm::length(glm::vec3(mat[0].x, mat[0].y, mat[0].z)),
					glm::length(glm::vec3(mat[1].x, mat[1].y, mat[1].z)),
					glm::length(glm::vec3(mat[2].x, mat[2].y, mat[2].z)));
	
	glm::mat3 rotationMatrix(
							 glm::vec3(mat[0].x, mat[0].y, mat[0].z) / scale.x,
							 glm::vec3(mat[1].x, mat[1].y, mat[1].z) / scale.y,
							 glm::vec3(mat[2].x, mat[2].y, mat[2].z) / scale.z);
	
	glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
	ozz::math::Quaternion rotation(rotationQuat.w, rotationQuat.x, rotationQuat.y, rotationQuat.z);
	
	return ozz::math::Transform{
		translation,         // ozz::math::Float3 (translation)
		rotation,            // ozz::math::Quaternion (rotation)
		ozz::math::Float3(scale.x, scale.y, scale.z) // ozz::math::Float3 (scale)
	};
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
	glm::mat4 rotationMatrix = GetUpAxisRotation(
												 mesh->document().global_settings.up_axis,
												 mesh->document().global_settings.up_axis_sign);
	
	// Apply unit scaling
	float scaleFactor = static_cast<float>(mesh->document().global_settings.unit_scale);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
	
	glm::mat4 transformMatrix = rotationMatrix * scaleMatrix;
	
	auto geometry = mesh->getGeometry();
	auto points = geometry->getPointsDeformed();
	auto vertexIndices = geometry->getIndices();
	
	// Process UV layers
	auto uvLayers = geometry->getUVLayers();
	
	const auto& materials = mesh->getMaterials();
	
	std::unordered_map<std::shared_ptr<sfbx::Material>, int64_t> materialIndexMap;
	
	for (size_t i = 0; i < materials.size(); ++i) {
		materialIndexMap[materials[i]] = static_cast<int64_t>(i);
	}
	
	std::vector<std::shared_ptr<sfbx::Material>> indexMaterialMap(materials.begin(), materials.end());
	
	// Reserve space for vertices and indices to avoid reallocations
	resultMesh->mVertices.resize(vertexIndices.size());
	resultMesh->mIndices.resize(vertexIndices.size());
	
	// Preprocess normal layers
	const auto& normalLayers = geometry->getNormalLayers();
	const bool hasNormals = !normalLayers.empty();
	const auto* normalLayer = hasNormals ? &normalLayers[0] : nullptr;
	
	// Precompute normal indices if needed
	std::vector<int> normalIndices;
	if (hasNormals && normalLayer->reference_mode == sfbx::LayerReferenceMode::IndexToDirect) {
		normalIndices = std::vector<int>(normalLayer->indices.begin(), normalLayer->indices.end());
	}
	
	// Preprocess UV indices
	std::vector<std::vector<int>> uvIndices(uvLayers.size());
	for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
		if (uvLayers[layerIndex].reference_mode == sfbx::LayerReferenceMode::IndexToDirect) {
			
			uvIndices[layerIndex] = std::vector<int>(uvLayers[layerIndex].indices.begin(), uvLayers[layerIndex].indices.end());
		}
	}
	
	// Process vertices
	for (size_t i = 0; i < vertexIndices.size(); ++i) {
		int controlPointIndex = vertexIndices[i];
		SkinnedMesh::Vertex& vertex = resultMesh->mVertices[i];
		
		// Set position
		const auto& point = points[controlPointIndex];
		glm::vec4 position(point.x, point.y, point.z, 1.0f);
		
		// Apply transformation (rotation and scaling)
		position = transformMatrix * position;
		vertex.set_position({position.x, position.y, position.z});
		
		// Process normals
		if (hasNormals) {
			int normalIndex = -1;
			switch (normalLayer->mapping_mode) {
				case sfbx::LayerMappingMode::ByPolygonVertex:
					normalIndex = (normalLayer->reference_mode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : normalIndices[i];
					break;
				case sfbx::LayerMappingMode::ByControlPoint:
					normalIndex = (normalLayer->reference_mode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : normalIndices[controlPointIndex];
					break;
				default:
					normalIndex = -1;
					break;
			}
			
			if (normalIndex >= 0 && normalIndex < normalLayer->data.size()) {
				const auto& normal = normalLayer->data[normalIndex];
				glm::vec4 normalVec(normal.x, normal.y, normal.z, 0.0f);
				
				// Apply rotation (do not apply scaling to normals)
				normalVec = rotationMatrix * normalVec;
				normalVec = glm::normalize(normalVec);
				vertex.set_normal({normalVec.x, normalVec.y, normalVec.z});
			} else {
				vertex.set_normal({0.0f, 1.0f, 0.0f});
			}
		} else {
			vertex.set_normal({0.0f, 1.0f, 0.0f});
		}
		
		// Process UV layers
		for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
			const auto& uvLayer = uvLayers[layerIndex];
			int uvIndex = -1;
			
			switch (uvLayer.mapping_mode) {
				case sfbx::LayerMappingMode::ByPolygonVertex:
					uvIndex = (uvLayer.reference_mode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : uvIndices[layerIndex][i];
					break;
				case sfbx::LayerMappingMode::ByControlPoint:
					uvIndex = (uvLayer.reference_mode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : uvIndices[layerIndex][controlPointIndex];
					break;
				default:
					uvIndex = -1;
					break;
			}
			
			if (uvIndex >= 0 && uvIndex < uvLayer.data.size()) {
				float u = uvLayer.data[uvIndex].x;
				float v = uvLayer.data[uvIndex].y;
				if (layerIndex == 0) {
					vertex.set_texture_coords1({u, v});
				} else if (layerIndex == 1) {
					vertex.set_texture_coords2({u, v});
				}
			} else {
				if (layerIndex == 0) {
					vertex.set_texture_coords1({0.0f, 0.0f});
				} else if (layerIndex == 1) {
					vertex.set_texture_coords2({0.0f, 0.0f});
				}
			}
		}
		
		// Get material for this vertex
		int materialIndex = geometry->getMaterialForVertexIndex(static_cast<int>(i));
		if (materialIndex >= 0 && materialIndex < static_cast<int>(indexMaterialMap.size())) {
			auto material = indexMaterialMap[materialIndex];
			int textureId = static_cast<int>(materialIndexMap[material]);
			vertex.set_texture_id(textureId);
		} else {
			vertex.set_texture_id(-1);
		}
		
		resultMesh->mIndices[i] = static_cast<uint32_t>(i);
	}
	
	// Process bones for skinning
	ProcessBones(mesh);
	
	// Process materials
	resultMesh->mMaterials.reserve(materialIndexMap.size());
	for (auto& [material, index] : materialIndexMap) {
		MaterialProperties matData;
		
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
				matData.mTextureDiffuse = std::make_unique<nanogui::Texture>(
																			 fbxTexture->getData().data(),
																			 static_cast<int>(fbxTexture->getData().size()));
				matData.mHasDiffuseTexture = true;
			}
		}
		
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
			std::string boneName = std::string { skinCluster->getChild()->getName() };
			auto [it, inserted] = mBoneMapping.emplace(boneName, static_cast<int>(mBoneMapping.size()));
			if (inserted) {
				mBoneTransforms.emplace_back(ToOzzTransform(skinCluster->getTransform()));
			}
			
			int boneID = it->second;
			const auto& weights = skinCluster->getWeights();
			const auto& indices = skinCluster->getIndices();
			
			for (size_t i = 0; i < weights.size(); ++i) {
				int vertexID = indices[i];
				float weight = weights[i];
				
				auto& vertex = mMeshes.back()->mVertices[vertexID];
				auto vertexWeights = vertex.get_weights();
				auto vertexBones = vertex.get_bone_ids();
				
				for (int j = 0; j < 4; ++j) {
					if (vertexWeights[j] == 0.0f) {
						vertexWeights[j] = weight;
						vertexBones[j] = boneID;
						break;
					}
				}
			}
		}
	}
}
