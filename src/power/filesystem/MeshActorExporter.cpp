// MeshActorExporter.cpp

#include "MeshActorExporter.hpp"
#include "VectorConversion.hpp"
#include <iostream>

MeshActorExporter::MeshActorExporter() {
	// Initialize the document (already done via constructor)
}

MeshActorExporter::~MeshActorExporter() {
	// Document will clean up automatically
}

bool MeshActorExporter::exportActor(const MeshActorImporter::CompressedMeshActor& actor, const std::string& exportPath) {
	// Step 1: Deserialize or transfer data from CompressedMeshActor to internal structures
	// Implement actual deserialization based on your serialization logic
	
	// Example Deserialization (Placeholder)
	// You need to implement the deserialization functions based on your CompressedSerialization::Serializer
	/*
	 CompressedSerialization::Deserializer deserializer(*actor.mMesh.mSerializer);
	 if (!deserializeMeshes(deserializer)) {
	 std::cerr << "Error: Failed to deserialize meshes." << std::endl;
	 return false;
	 }
	 
	 if (!deserializeSkeleton(deserializer)) {
	 std::cerr << "Error: Failed to deserialize skeleton." << std::endl;
	 return false;
	 }
	 */
	
	// For demonstration purposes, let's assume you have functions to deserialize
	// These functions need to be implemented based on your CompressedSerialization::Serializer
	
	// TODO: Implement deserialization from actor to mMeshes and mSkeleton
	// Example:
	// mMeshes = actor.getMeshes();
	// mSkeleton = actor.getSkeleton();
	
	// Step 2: Create Materials
	std::map<int, std::shared_ptr<sfbx::Material>> materialMap; // Map textureID to Material
	if (!mMeshes.empty()) {
		// Upcast shared_ptr<MaterialProperties> to shared_ptr<SerializableMaterialProperties>
		std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
		serializableMaterials.reserve(mMeshes[0]->get_material_properties().size());
		
		for (const auto& mat : mMeshes[0]->get_material_properties()) {
			// Ensure that mat is indeed a shared_ptr<SerializableMaterialProperties>
			serializableMaterials.emplace_back(std::make_shared<SerializableMaterialProperties>(*mat));
		}
		
		createMaterials(serializableMaterials, materialMap); // Assuming all meshes share the same materials
	}
	
	// Step 3: Create Meshes and Models
	for (size_t i = 0; i < mMeshes.size(); ++i) {
		auto& meshData = *mMeshes[i];
		std::shared_ptr<sfbx::Mesh> meshModel = mDocument.createObject<sfbx::Mesh>("Mesh_" + std::to_string(i));
		assert(meshModel != nullptr);
		createMesh(meshData, materialMap, meshModel);
	}
	
	// Step 5: Skip Animations as per user request
	
	// Step 6: Export the Document to a file
	if (!mDocument.writeBinary(exportPath)) {
		std::cerr << "Error: Failed to write FBX file to " << exportPath << std::endl;
		return false;
	}
	
	std::cout << "Successfully exported actor to " << exportPath << std::endl;
	return true;
}
void MeshActorExporter::createMaterials(
										const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials,
										std::map<int, std::shared_ptr<sfbx::Material>>& materialMap
										) {
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto& matProps = materials[i];
		std::shared_ptr<sfbx::Material> material = mDocument.createObject<sfbx::Material>("Material_" + std::to_string(i));
		assert(material != nullptr);
		
		// Set material properties using helper functions
		material->setAmbientColor(sfbx::glmToDouble3(glm::vec3(matProps->mAmbient)));
		material->setDiffuseColor(sfbx::glmToDouble3(glm::vec3(matProps->mDiffuse)));
		material->setSpecularColor(sfbx::glmToDouble3(glm::vec3(matProps->mSpecular)));
		material->setShininess(matProps->mShininess);
		material->setOpacity(matProps->mOpacity);
		
		// Handle textures
		if (matProps->mHasDiffuseTexture) {
			std::shared_ptr<sfbx::Texture> texture = mDocument.createObject<sfbx::Texture>("Texture_" + std::to_string(i));
			assert(texture != nullptr);
			texture->setData(matProps->mTextureDiffuse); // Ensure setFilename accepts std::vector<uint8_t>
			texture->setEmbedded(true); // Set based on your requirements
			// Optionally, load texture data here if necessary
			material->setTexture("Diffuse", texture);
		}
		
		materialMap[i] = material;
	}
}

void MeshActorExporter::createMesh(
								   MeshData& meshData,
								   const std::map<int, std::shared_ptr<sfbx::Material>>& materialMap,
								   std::shared_ptr<sfbx::Mesh> parentModel
								   ) {
	auto geometryName = "Geometry_" + std::string{ parentModel->getName() };
	std::shared_ptr<sfbx::GeomMesh> geometry = mDocument.createObject<sfbx::GeomMesh>(geometryName);
	assert(geometry != nullptr);
	
	// Set control points (vertices)
	for (auto& vertex : meshData.get_vertices()) {
		geometry->addControlPoint(vertex.get_position().x, vertex.get_position().y, vertex.get_position().z);
	}
	
	// Set indices (polygons)
	auto& indices = meshData.get_indices();
	for (size_t i = 0; i + 2 < indices.size(); i += 3) { // Assuming triangles
		geometry->addPolygon(indices[i], indices[i + 1], indices[i + 2]);
	}
	
	// **Add Layers Here**
	
	// 1. Normals Layer
	{
		sfbx::LayerElementF3 normalLayer;
		normalLayer.name = "Normals";
		normalLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygonVertex;
		normalLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		for (auto& vertex : meshData.get_vertices()) {
			normalLayer.data.emplace_back(vertex.get_normal().x, vertex.get_normal().y, vertex.get_normal().z);
		}
		
		geometry->addNormalLayer(std::move(normalLayer));
	}
	
	// 2. UV Layers (assuming two UV sets)
	for (int uvSet = 0; uvSet < 2; ++uvSet) {
		sfbx::LayerElementF2 uvLayer;
		uvLayer.name = "UVSet" + std::to_string(uvSet + 1);
		uvLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygonVertex;
		uvLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		for (auto& vertex : meshData.get_vertices()) {
			if (uvSet == 0) {
				uvLayer.data.emplace_back(vertex.get_tex_coords1().x, vertex.get_tex_coords1().y);
			} else {
				uvLayer.data.emplace_back(vertex.get_tex_coords2().x, vertex.get_tex_coords2().y);
			}
		}
		
		geometry->addUVLayer(std::move(uvLayer));
	}
	
	// 3. Vertex Colors Layer
	{
		sfbx::LayerElementF4 colorLayer;
		colorLayer.name = "VertexColors";
		colorLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygonVertex;
		colorLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		for (auto& vertex : meshData.get_vertices()) {
			colorLayer.data.emplace_back(
										 vertex.get_color().r,
										 vertex.get_color().g,
										 vertex.get_color().b,
										 vertex.get_color().a
										 );
		}
		
		geometry->addColorLayer(std::move(colorLayer));
	}
	
	// **End of Layers Addition**
	
	// **Material Layer**
	{
		sfbx::LayerElementI1 materialLayer;
		materialLayer.name = "Materials";
		materialLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygon;
		materialLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		size_t numPolygons = geometry->getIndices().size() / 3; // Assuming triangles
		materialLayer.data.reserve(numPolygons);
		
		for (size_t polyIdx = 0; polyIdx < numPolygons; ++polyIdx) {
			// Determine the material index for this polygon
			// This requires that meshData has a way to get the material index per polygon
			// For example, meshData.get_material_index(polyIdx)
			// If meshData does not support this, you need to implement it accordingly
			
			// Placeholder: Assign material based on some logic
			// Here, we assume that all polygons use the first material
			// Replace this with actual material assignment logic
			int materialIndex = 0; // Default material index
			
			// Example logic: If meshData contains material indices per polygon
			if (polyIdx < meshData.get_indices().size()) {
				materialIndex = meshData.get_indices()[polyIdx];
			}
			
			// Ensure the materialIndex exists in materialMap
			if (materialMap.find(materialIndex) != materialMap.end()) {
				materialLayer.data.push_back(materialIndex);
			} else {
				std::cerr << "Warning: Material index " << materialIndex << " not found in materialMap. Assigning default material 0." << std::endl;
				materialLayer.data.push_back(0); // Assign default material
			}
		}
		
		geometry->addMaterialLayer(std::move(materialLayer));
	}
	
	// **Link geometry to model**
	parentModel->setGeometry(geometry);
}

// Deserialization functions (to be implemented based on your serialization logic)
bool MeshActorExporter::deserializeMeshes(const CompressedSerialization::Deserializer& deserializer) {
	// Implement mesh deserialization here
	return true; // Placeholder
}

bool MeshActorExporter::deserializeSkeleton(const CompressedSerialization::Deserializer& deserializer) {
	// Implement skeleton deserialization here
	return true; // Placeholder
}
