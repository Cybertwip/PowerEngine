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

}  // namespace FbxUtil

void Fbx::LoadModel(const std::string& path) {
	mDoc = sfbx::MakeDocument(path);
	
	if (mDoc && mDoc->valid()) {
		ProcessNode(mDoc->getRootModel());
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	mDoc = sfbx::MakeDocument(data);
	
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
	
	// Reserve space for vertices and indices
	const size_t vertexCount = points.size();
	const size_t indexCount = vertexIndices.size();
	resultMesh->get_vertices().resize(indexCount);
	resultMesh->get_indices().resize(indexCount);
	
	// Retrieve materials and map them to indices
	const auto& materials = mesh->getMaterials();
	
	// Precompute normal indices
	const auto& normalLayers = geometry->getNormalLayers();
	std::vector<int> normalIndices(indexCount, -1);
	sfbx::RawVector<sfbx::double3> normalData;
	if (!normalLayers.empty()) {
		const auto& normalLayer = normalLayers[0];
		normalData = normalLayer.data;
		const auto mappingMode = normalLayer.mapping_mode;
		const auto referenceMode = normalLayer.reference_mode;
		const auto& normalLayerIndices = normalLayer.indices;
		
		for (size_t i = 0; i < indexCount; ++i) {
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
	std::vector<std::vector<int>> uvIndicesPerLayer(uvLayers.size(), std::vector<int>(indexCount, -1));
	for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
		const auto& uvLayer = uvLayers[layerIndex];
		const auto mappingMode = uvLayer.mapping_mode;
		const auto referenceMode = uvLayer.reference_mode;
		const auto& uvLayerIndices = uvLayer.indices;
		
		for (size_t i = 0; i < indexCount; ++i) {
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
	std::vector<int> colorIndices(indexCount, -1);
	sfbx::RawVector<sfbx::double4> colorData; // Assuming colors are RGBA
	if (!colorLayers.empty()) {
		const auto& colorLayer = colorLayers[0]; // Use the first color layer
		colorData = colorLayer.data;
		const auto mappingMode = colorLayer.mapping_mode;
		const auto referenceMode = colorLayer.reference_mode;
		const auto& colorLayerIndices = colorLayer.indices;
		
		for (size_t i = 0; i < indexCount; ++i) {
			int controlPointIndex = vertexIndices[i];
			if (mappingMode == sfbx::LayerMappingMode::ByPolygonVertex) {
				colorIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? static_cast<int>(i) : colorLayerIndices[i];
			} else if (mappingMode == sfbx::LayerMappingMode::ByControlPoint) {
				colorIndices[i] = (referenceMode == sfbx::LayerReferenceMode::Direct) ? controlPointIndex : colorLayerIndices[controlPointIndex];
			}
		}
	}
	
	// Determine the number of threads to use
	unsigned int numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) numThreads = 1;
	
	// Divide the work among threads
	std::vector<std::thread> threads(numThreads);
	size_t chunkSize = indexCount / numThreads;
	size_t remainder = indexCount % numThreads;
	
	auto processVertices = [&](size_t startIndex, size_t endIndex) {
		for (size_t i = startIndex; i < endIndex; ++i) {
			int controlPointIndex = vertexIndices[i];
			
			// Create a new MeshVertex per index
			auto& vertexPtr = resultMesh->get_vertices()[i];
			vertexPtr = std::make_unique<MeshVertex>();
			auto& vertex = *vertexPtr;
			
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
				vertex.set_normal({ 0.0f, 1.0f, 0.0f }); // Default normal
			}
			
			// Set UVs
			for (size_t layerIndex = 0; layerIndex < uvLayers.size(); ++layerIndex) {
				int uvIndex = uvIndicesPerLayer[layerIndex][i];
				if (uvIndex >= 0 && static_cast<size_t>(uvIndex) < uvLayers[layerIndex].data.size()) {
					const auto& uv = uvLayers[layerIndex].data[uvIndex];
					
					// Apply transformation if needed
					glm::vec2 finalUV(uv.x, uv.y);
					
					// Optionally clamp the UVs between 0 and 1
					finalUV = glm::fract(finalUV);
					
					// Flip Y-axis if needed
					// finalUV.y = 1.0f - finalUV.y;
					
					if (layerIndex == 0) {
						vertex.set_texture_coords1(finalUV);
					} else if (layerIndex == 1) {
						vertex.set_texture_coords2(finalUV);
					}
				} else {
					vertex.set_texture_coords1(glm::vec2(0.0f, 0.0f)); // Default UV
					vertex.set_texture_coords2(glm::vec2(0.0f, 0.0f)); // Default UV
				}
			}
			
			// Assign Vertex Color
			if (!colorData.empty() && colorIndices[i] >= 0) {
				const auto& color = colorData[colorIndices[i]];
				vertex.set_color({ color.x, color.y, color.z, color.w }); // Including alpha channel
			} else {
				vertex.set_color({ 1.0f, 1.0f, 1.0f, 1.0f }); // Default color (white)
			}
			
			// Set material ID
			int matIndex = geometry->getMaterialForPolygonIndex(i / 3); // Assuming triangles
			if (matIndex >= 0 && matIndex < static_cast<int>(materials.size())) {
				vertex.set_material_id(matIndex);
			} else {
				vertex.set_material_id(0); // Default material ID
			}
			
			// Set the index
			resultMesh->get_indices()[i] = static_cast<unsigned int>(i);
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
	 ProcessBones(mesh); // Uncomment if you have a ProcessBones function
}
