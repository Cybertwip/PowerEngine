#include "import/Fbx.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace FbxUtil {

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

}  // namespace

Fbx::Fbx(const std::string& path) : mPath(path) {
}

void Fbx::LoadModel() {
	mDoc = sfbx::MakeDocument(std::string(mPath));

    if (mDoc && mDoc->valid()) {
        ProcessNode(mDoc->getRootModel());
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
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	// Retrieve the absolute path of the mesh document
	auto path = std::filesystem::absolute(mesh->document()->global_settings.path).string();
	
	// Create a new MeshData instance and add it to mMeshes
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	
	// Precompute transformation matrices
	const glm::mat4 rotationMatrix = FbxUtil::GetUpAxisRotation(
																mesh->document()->global_settings.up_axis,
																mesh->document()->global_settings.up_axis_sign
																);
	const float scaleFactor = static_cast<float>(mesh->document()->global_settings.unit_scale);
	const glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
	const glm::mat4 transformMatrix = rotationMatrix * scaleMatrix;
	
	// Retrieve geometry data
	auto geometry = mesh->getGeometry();
	if (!geometry) {
		std::cerr << "Error: Mesh has no geometry." << std::endl;
		return;
	}
	
	const auto& points = geometry->getPointsDeformed();
	const auto& vertexIndices = geometry->getIndices();
	const auto& indexCounts = geometry->getCounts();
	
	// Retrieve materials and map them to indices
	const auto& materials = mesh->getMaterials();
	
	// Precompute normal indices
	const auto& normalLayers = geometry->getNormalLayers();
	std::vector<int> normalIndices(vertexIndices.size(), -1);
	sfbx::RawVector<sfbx::double3> normalData;
	if (!normalLayers.empty()) {
		const auto& normalLayer = normalLayers[0];
		normalData = normalLayer.data;
		const auto mappingMode = normalLayer.mapping_mode;
		const auto referenceMode = normalLayer.reference_mode;
		const auto& normalLayerIndices = normalLayer.indices;
		
		for (size_t i = 0; i < vertexIndices.size(); ++i) {
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
	std::vector<std::vector<int>> uvIndicesPerLayer(uvLayers.size(), std::vector<int>(vertexIndices.size(), -1));
	for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
		const auto& uvLayer = uvLayers[layerIndex];
		const auto mappingMode = uvLayer.mapping_mode;
		const auto referenceMode = uvLayer.reference_mode;
		const auto& uvLayerIndices = uvLayer.indices;
		
		for (size_t i = 0; i < vertexIndices.size(); ++i) {
			int controlPointIndex = vertexIndices[i];
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				uvIndicesPerLayer[layerIndex][i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : uvLayerIndices[i];
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				uvIndicesPerLayer[layerIndex][i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : uvLayerIndices[controlPointIndex];
			}
		}
	}
	
	
	// Precompute Color Indices
	const auto& colorLayers = geometry->getColorLayers();
	std::vector<int> colorIndices(vertexIndices.size(), -1);
	sfbx::RawVector<sfbx::double4> colorData; // Assuming colors are RGBA
	if (!colorLayers.empty()) {
		const auto& colorLayer = colorLayers[0]; // Use the first color layer
		colorData = colorLayer.data;
		const auto mappingMode = colorLayer.mapping_mode;
		const auto referenceMode = colorLayer.reference_mode;
		const auto& colorLayerIndices = colorLayer.indices;
		
		for (size_t i = 0; i < vertexIndices.size(); ++i) {
			int controlPointIndex = vertexIndices[i];
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				colorIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : colorLayerIndices[i];
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				colorIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : colorLayerIndices[controlPointIndex];
			}
		}
	}
	
	// Use a hash map to deduplicate vertices based on position, normal, and UVs
	struct VertexKey {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv1;
		glm::vec2 uv2;
		bool operator<(const VertexKey& other) const {
			// Compare positions
			if (position.x != other.position.x) return position.x < other.position.x;
			if (position.y != other.position.y) return position.y < other.position.y;
			if (position.z != other.position.z) return position.z < other.position.z;
			
			// Compare normals
			if (normal.x != other.normal.x) return normal.x < other.normal.x;
			if (normal.y != other.normal.y) return normal.y < other.normal.y;
			if (normal.z != other.normal.z) return normal.z < other.normal.z;
			
			// Compare UV1
			if (uv1.x != other.uv1.x) return uv1.x < other.uv1.x;
			if (uv1.y != other.uv1.y) return uv1.y < other.uv1.y;
			
			// Compare UV2
			if (uv2.x != other.uv2.x) return uv2.x < other.uv2.x;
			if (uv2.y != other.uv2.y) return uv2.y < other.uv2.y;
			
			// All components are equal
			return false;
		}
	};
	
	std::map<VertexKey, uint32_t> vertexMap;
	uint32_t newIndex = 0;
	
	// Reserve space
	resultMesh->get_vertices().reserve(vertexIndices.size());
	resultMesh->get_indices().reserve(vertexIndices.size());
	
	// Iterate over each index to construct unique vertices
	for (size_t i = 0; i < vertexIndices.size(); ++i) {
		int controlPointIndex = vertexIndices[i];
		
		// Ensure controlPointIndex is within bounds
		if (controlPointIndex >= points.size()) {
			sfbxPrint("Error: controlPointIndex %d out of bounds.\n", controlPointIndex);
			continue;
		}
		
		// Get position
		const auto& point = points[controlPointIndex];
		glm::vec4 position(point.x, point.y, point.z, 1.0f);
		position = transformMatrix * position;
		glm::vec3 transformedPos = glm::vec3(position);
		
		// Get normal
		glm::vec3 transformedNormal(0.0f, 1.0f, 0.0f); // Default normal
		if (!normalData.empty() && normalIndices[i] >= 0 && static_cast<size_t>(normalIndices[i]) < normalData.size()) {
			const auto& normal = normalData[normalIndices[i]];
			glm::vec4 normalVec(normal.x, normal.y, normal.z, 0.0f);
			normalVec = rotationMatrix * normalVec;
			transformedNormal = glm::normalize(glm::vec3(normalVec));
		}
		
		// Get UVs
		glm::vec2 uv1(0.0f, 0.0f);
		glm::vec2 uv2(0.0f, 0.0f);
		if (!uvLayers.empty()) {
			// First UV layer
			int uvIndex1 = uvIndicesPerLayer[0][i];
			if (uvIndex1 >= 0 && static_cast<size_t>(uvIndex1) < uvLayers[0].data.size()) {
				const auto& uv = uvLayers[0].data[uvIndex1];
				uv1 = glm::vec2(uv.x, uv.y);
			}
			// Second UV layer, if available
			if (uvLayers.size() > 1) {
				int uvIndex2 = uvIndicesPerLayer[1][i];
				if (uvIndex2 >= 0 && static_cast<size_t>(uvIndex2) < uvLayers[1].data.size()) {
					const auto& uv = uvLayers[1].data[uvIndex2];
					uv2 = glm::vec2(uv.x, uv.y);
				}
			}
		}
		
		// Get color
		glm::vec4 color(1.0f);
		if (!colorData.empty() && colorIndices[i] >= 0 && static_cast<size_t>(colorIndices[i]) < colorData.size()) {
			const auto& col = colorData[colorIndices[i]];
			color = glm::vec4(col.x, col.y, col.z, col.w);
		}
		
		// Create a key for deduplication
		VertexKey key{
			transformedPos,
			transformedNormal,
			uv1,
			uv2
		};
		
		// Check if the vertex already exists
		auto it = vertexMap.find(key);
		if (it != vertexMap.end()) {
			// Vertex exists, reuse the index
			resultMesh->get_indices().push_back(it->second);
		} else {
			// New unique vertex
			auto vertexPtr = std::make_unique<MeshVertex>();
			vertexPtr->set_position(transformedPos);
			vertexPtr->set_normal(transformedNormal);
			vertexPtr->set_texture_coords1(uv1);
			vertexPtr->set_texture_coords2(uv2);
			vertexPtr->set_color(color);
			
			// Assign material ID
			int matIndex = geometry->getMaterialForVertexIndex(controlPointIndex);
			if (matIndex >= 0 && matIndex < static_cast<int>(materials.size())) {
				vertexPtr->set_material_id(matIndex);
			} else {
				vertexPtr->set_material_id(0); // or a default material ID
			}
			
			// Add the new vertex to the vertex buffer
			resultMesh->get_vertices().push_back(std::move(vertexPtr));
			
			// Map the key to the new index
			vertexMap[key] = newIndex;
			
			// Add the index
			resultMesh->get_indices().push_back(newIndex);
			
			// Increment the index
			newIndex++;
		}
	}
	
	// Process materials
	std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
	serializableMaterials.reserve(materials.size());
	
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto& material = materials[i];
		auto matPtr = std::make_shared<SerializableMaterialProperties>();
		SerializableMaterialProperties& matData = *matPtr;
		
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
		matData.mSpecular = glm::vec4{ color.x, color.y, color.z, 1.0f };
		matData.mShininess = 0.1f; // Default shininess; adjust as needed
		matData.mOpacity = material->getOpacity();
		matData.mHasDiffuseTexture = false;
		
		// Load texture if available
		if (auto fbxTexture = material->getTexture("DiffuseColor"); fbxTexture && !fbxTexture->getData().empty()) {
			matData.mTextureDiffuse = fbxTexture->getData();
			matData.mHasDiffuseTexture = true;
		}
		
		serializableMaterials.push_back(matPtr);
	}
	
	resultMesh->get_material_properties().resize(serializableMaterials.size());
	mMaterialProperties.push_back(std::move(serializableMaterials));
	
	// Proceed to process bones separately
	ProcessBones(mesh); // This call should be handled externally or after ProcessMesh
}
