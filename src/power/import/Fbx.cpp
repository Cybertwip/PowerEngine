#include "import/Fbx.hpp"

#include <algorithm>
#include <execution>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>
#include <functional> // For std::hash
#include <iostream> // For debugging

namespace {

// Improved hash_combine function inspired by Boost
inline std::size_t hash_combine(std::size_t seed, std::size_t value) {
	return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

inline std::size_t combineHashes(std::size_t h1, std::size_t h2, std::size_t h3, std::size_t h4, std::size_t h5,
								 std::size_t h6, std::size_t h7, std::size_t h8, std::size_t h9, std::size_t h10,
								 std::size_t h11, std::size_t h12, std::size_t h13, std::size_t h14, std::size_t h15) {
	std::size_t seed = h1;
	seed = hash_combine(seed, h2);
	seed = hash_combine(seed, h3);
	seed = hash_combine(seed, h4);
	seed = hash_combine(seed, h5);
	seed = hash_combine(seed, h6);
	seed = hash_combine(seed, h7);
	seed = hash_combine(seed, h8);
	seed = hash_combine(seed, h9);
	seed = hash_combine(seed, h10);
	seed = hash_combine(seed, h11);
	seed = hash_combine(seed, h12);
	seed = hash_combine(seed, h13);
	seed = hash_combine(seed, h14);
	seed = hash_combine(seed, h15);
	return seed;
}

constexpr float Quantize(float value, float epsilon = 1e-5f) {
	return std::round(value / epsilon) * epsilon;
}

struct VertexKey {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv1;
	glm::vec2 uv2;
	glm::vec4 color;
	uint32_t material_id;
	
	bool operator==(const VertexKey& other) const {
		return position == other.position &&
		normal == other.normal &&
		uv1 == other.uv1 &&
		uv2 == other.uv2 &&
		color == other.color &&
		material_id == other.material_id;
	}
};

// Custom hash function using size_t and improved hash combination
struct VertexKeyHash {
	std::size_t operator()(const VertexKey& key) const {
		std::size_t h1 = std::hash<float>()(key.position.x);
		std::size_t h2 = std::hash<float>()(key.position.y);
		std::size_t h3 = std::hash<float>()(key.position.z);
		std::size_t h4 = std::hash<float>()(key.normal.x);
		std::size_t h5 = std::hash<float>()(key.normal.y);
		std::size_t h6 = std::hash<float>()(key.normal.z);
		std::size_t h7 = std::hash<float>()(key.uv1.x);
		std::size_t h8 = std::hash<float>()(key.uv1.y);
		std::size_t h9 = std::hash<float>()(key.uv2.x);
		std::size_t h10 = std::hash<float>()(key.uv2.y);
		std::size_t h11 = std::hash<float>()(key.color.x);
		std::size_t h12 = std::hash<float>()(key.color.y);
		std::size_t h13 = std::hash<float>()(key.color.z);
		std::size_t h14 = std::hash<float>()(key.color.w);
		std::size_t h15 = std::hash<uint32_t>()(key.material_id);
		
		// Combine all hashes using the improved hash_combine
		return combineHashes(h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14, h15);
	}
};

} // namespace

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

}  // namespace FbxUtil

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
	
	// Clear previous data
	resultMesh->get_vertices().clear();
	resultMesh->get_indices().clear();
	
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
			}
			else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
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
			}
			else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
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
			}
			else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				colorIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : colorLayerIndices[controlPointIndex];
			}
		}
	}
	
	// Initialize unordered_map for deduplication
	std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
	uint32_t newIndex = 0;
	
	// Reserve space
	resultMesh->get_vertices().reserve(vertexIndices.size());
	resultMesh->get_indices().reserve(vertexIndices.size());
	
	// Iterate over each index to construct unique vertices
	for (size_t i = 0; i < vertexIndices.size(); ++i) {
		int controlPointIndex = vertexIndices[i];
		
		// Enhanced index validation
		if (controlPointIndex >= static_cast<int>(points.size()) || controlPointIndex < 0) {
			sfbxPrint("Error: controlPointIndex %d out of bounds. Points size: %zu\n", controlPointIndex, points.size());
			// Handle the error as needed
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
		
		// Get material ID
		int matIndex = geometry->getMaterialForVertexIndex(controlPointIndex);
		uint32_t materialID = 0; // Default material ID
		if (matIndex >= 0 && matIndex < static_cast<int>(materials.size())) {
			materialID = static_cast<uint32_t>(matIndex);
		}
		
		// Create a key for deduplication with quantization
		VertexKey key{
			glm::vec3(Quantize(transformedPos.x), Quantize(transformedPos.y), Quantize(transformedPos.z)),
			glm::vec3(Quantize(transformedNormal.x), Quantize(transformedNormal.y), Quantize(transformedNormal.z)),
			glm::vec2(uv1.x, uv1.y),
			glm::vec2(uv2.x, uv2.y),
			color,
			materialID
		};
		
		// Check if the vertex already exists
		auto it = vertexMap.find(key);
		if (it != vertexMap.end()) {
			// Vertex exists, reuse the index
			resultMesh->get_indices().push_back(it->second);
		}
		else {
			// New unique vertex
			auto vertexPtr = std::make_unique<MeshVertex>();
			vertexPtr->set_position(transformedPos);
			vertexPtr->set_normal(transformedNormal);
			vertexPtr->set_texture_coords1(uv1);
			vertexPtr->set_texture_coords2(uv2);
			vertexPtr->set_color(color);
			vertexPtr->set_material_id(materialID);
			
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
	ProcessBones(mesh); // Ensure this is handled appropriately
}
