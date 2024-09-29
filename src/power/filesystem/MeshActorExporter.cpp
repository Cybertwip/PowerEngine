// MeshActorExporter.cpp

#include "MeshActorExporter.hpp"
#include "MeshDeserializer.hpp"
#include "VectorConversion.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>

namespace ExporterUtil {


// return translate, rotate, scale
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4 &transform)
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, rotation, scale};
}
glm::vec3 ExtractScale(const glm::mat4& matrix) {
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));
	return scale;
}


inline glm::mat4 SfbxMatToGlmMat(const sfbx::double4x4 &from)
{
	glm::mat4 to;
	// the a,b,c,d in sfbx is the row ; the 1,2,3,4 is the column
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			to[j][i] = from[j][i];
		}
	}
	
	return to;
}

inline sfbx::double4x4 GlmMatToSfbxMat(const glm::mat4 &from)
{
	sfbx::double4x4 to;
	// the a,b,c,d in sfbx is the row ; the 1,2,3,4 is the column
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			to[j][i] = from[j][i];
		}
	}
	
	return to;
}

}  // namespace

MeshActorExporter::MeshActorExporter()
{
	
	mMeshDeserializer = std::make_unique<MeshDeserializer>();

	// Initialize the document (already done via constructor)
}

MeshActorExporter::~MeshActorExporter() {
	// Document will clean up automatically
}

bool MeshActorExporter::exportActor(CompressedSerialization::Deserializer& deserializer, const std::string& sourcePath, const std::string& exportPath) {
	
	mMeshModels.clear();
	
	auto document = sfbx::MakeDocument();

	// Step 1: Deserialize or transfer data from CompressedMeshActor to internal structures
	mMeshDeserializer->load_mesh(deserializer, sourcePath);
	// Retrieve the deserialized meshes
	mMeshes = std::move(mMeshDeserializer->get_meshes());
	
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
		
		createMaterials(document, serializableMaterials, materialMap); // Assuming all meshes share the same materials
	}
	
	
	auto rootModel = document->getRootModel();

	// Step 3: Create Meshes and Models
	// After creating the mesh
	for (size_t i = 0; i < mMeshes.size(); ++i) {
		auto& meshData = *mMeshes[i];
		
		std::shared_ptr<sfbx::Mesh> meshModel = std::make_shared<sfbx::Mesh>();
		
		rootModel->addChild(meshModel); // to set m_document

		meshModel->setName("Mesh_" + std::to_string(i));
		
		
		assert(meshModel != nullptr);
		createMesh(meshData, materialMap, meshModel);
		
		// Create a node for each mesh
		
		// Set transformations on the meshModel
		meshModel->setName("Node_" + std::to_string(i));
		meshModel->setPosition({0.0f, 0.0f, 0.0f});
		meshModel->setRotation({0.0f, 0.0f, 0.0f});
		meshModel->setScale({1.0f, 1.0f, 1.0f});
		
		mMeshModels.push_back(meshModel);
	}

	// Step 5: Build skeleton
	
	auto skeleton = mMeshDeserializer->get_skeleton();
	
	if (skeleton != std::nullopt) {
		// Get the actual Skeleton object
		auto& skeletonData = *skeleton;
		
		// Map to store bone index to sfbx::Model
		std::map<int, std::shared_ptr<sfbx::LimbNode>> boneModels;
		
		// Step 1: Create sfbx::Model nodes for each bone
		for (int boneIndex = 0; boneIndex < skeletonData.num_bones(); ++boneIndex) {
			std::string boneName = skeletonData.get_bone(boneIndex).name;
			int parentIndex = skeletonData.get_bone(boneIndex).parent_index;
			
			// Create a new Model node for the bone
			auto boneModel = document->createObject<sfbx::LimbNode>(boneName);
			
			// Step 2: Set bone's local transformations
			glm::mat4 poseMatrix = skeletonData.get_bone(boneIndex).bindpose;
			
			// Set the local transformations for the boneModel
			boneModel->setLocalMatrix(ExporterUtil::GlmMatToSfbxMat(poseMatrix));

			// Store the bone model in the map
			boneModels[boneIndex] = boneModel;
		}
		
		// Step 3: Associate the skeleton with the mesh via skinning
		// Assuming mMeshModels is a vector of sfbx::Mesh models corresponding to mMeshes
		for (size_t i = 0; i < mMeshes.size(); ++i) {
			auto& meshData = *mMeshes[i];
			auto meshModel = mMeshModels[i];
			
			auto geometry = meshModel->getGeometry();
			if (!geometry) {
				std::cerr << "Error: Mesh model does not have a geometry." << std::endl;
				continue;
			}
			
			// Create a Skin deformer and add it to the geometry
			auto skin = document->createObject<sfbx::Skin>("Skin_" + std::string { meshModel->getName()});
			geometry->addChild(skin);
			
			// Map to store bone influences per bone
			std::map<int, std::vector<std::pair<int, float>>> boneWeights; // boneIndex -> [(vertexIndex, weight)]
			
			auto& vertices = meshData.get_vertices();
			
			// Collect bone weights from vertices
			for (size_t vi = 0; vi < vertices.size(); ++vi) {
				auto& vertex = static_cast<SkinnedMeshVertex&>(*vertices[vi]);
				
				// Assuming vertex provides bone IDs and weights
				auto boneIDs = vertex.get_bone_ids();    // std::vector<int>
				auto weights = vertex.get_weights();     // std::vector<float>
				
				for (size_t j = 0; j < boneIDs.size(); ++j) {
					int boneID = boneIDs[j];
					float weight = weights[j];
					
					if (boneID == -1){
						continue;
					}
					
					boneWeights[boneID].emplace_back(static_cast<int>(vi), weight);
				}
			}
			
			// Create clusters for each bone and associate them with the skin deformer
			for (const auto& [boneID, weightData] : boneWeights) {
				auto cluster = document->createObject<sfbx::Cluster>("Cluster_" + std::to_string(boneID));
				skin->addChild(cluster);
				
				// Extract indices and weights
				std::vector<int> indices;
				std::vector<float> weights;
				for (const auto& [vertexIndex, weight] : weightData) {
					indices.push_back(vertexIndex);
					weights.push_back(weight);
				}
				
				// Set indices and weights in the cluster
				cluster->setIndices(indices);
				cluster->setWeights(weights);
								
				// Set the link node to the corresponding bone model
				auto offsetMatrix = skeletonData.get_bone(boneID).offset;
				cluster->setTransform(ExporterUtil::GlmMatToSfbxMat(offsetMatrix));
				
				cluster->addChild(boneModels[boneID]);
			}
		}
	}

	// Step 6: Export the Document to a file
	document->exportFBXNodes();
	
	if (!document->writeAscii(exportPath)) {
		std::cerr << "Error: Failed to write FBX file to " << exportPath << std::endl;
		return false;
	}
	
	std::cout << "Successfully exported actor to " << exportPath << std::endl;
	return true;
}

void MeshActorExporter::createMaterials(std::shared_ptr<sfbx::Document> document, 
										const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials,
										std::map<int, std::shared_ptr<sfbx::Material>>& materialMap
										) {
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto& matProps = materials[i];
		std::shared_ptr<sfbx::Material> material = document->createObject<sfbx::Material>("Material_" + std::to_string(i));
		assert(material != nullptr);
		
		// Set material properties using helper functions
		material->setAmbientColor(sfbx::glmToDouble3(glm::vec3(matProps->mAmbient)));
		material->setDiffuseColor(sfbx::glmToDouble3(glm::vec3(matProps->mDiffuse)));
		material->setSpecularColor(sfbx::glmToDouble3(glm::vec3(matProps->mSpecular)));
		material->setShininess(matProps->mShininess);
		material->setOpacity(matProps->mOpacity);
		
		// Handle textures
		if (matProps->mHasDiffuseTexture) {
			std::shared_ptr<sfbx::Texture> texture = document->createObject<sfbx::Texture>("Texture_" + std::to_string(i));
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
	std::shared_ptr<sfbx::GeomMesh> geometry = std::make_shared<sfbx::GeomMesh>();
	
	geometry->setName(geometryName);
	
	parentModel->addChild(geometry);
	assert(geometry != nullptr);
	
	// Set control points (vertices)
	for (auto& vertexPtr : meshData.get_vertices()) {
		auto& vertex = *vertexPtr;
		
		geometry->addControlPoint(vertex.get_position().x, vertex.get_position().y, vertex.get_position().z);
	}

	// Set indices (polygons)
	auto& indices = meshData.get_indices();
	for (size_t i = 0; i + 2 < indices.size(); i += 3) { // Assuming triangles
		geometry->addPolygon(indices[i], indices[i + 1], indices[i + 2]);
	}
	
	// **Add Layers Here**
	/*
	// 1. Normals Layer
	{
		sfbx::LayerElementF3 normalLayer;
		normalLayer.name = "Normals";
		normalLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygonVertex;
		normalLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		for (auto& vertexPtr : meshData.get_vertices()) {
			auto& vertex = *vertexPtr;
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
		
		for (auto& vertexPtr : meshData.get_vertices()) {
			auto& vertex = *vertexPtr;
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
		
		for (auto& vertexPtr : meshData.get_vertices()) {
			auto& vertex = *vertexPtr;
			
			colorLayer.data.emplace_back(
										 vertex.get_color().r,
										 vertex.get_color().g,
										 vertex.get_color().b,
										 vertex.get_color().a
										 );
		}
		
		geometry->addColorLayer(std::move(colorLayer));
	}
	
	 //**End of Layers Addition**
	
	 //**Material Layer**
	
	for (auto material : materialMap) {
		sfbx::LayerElementI1 materialLayer;
		materialLayer.mapping_mode = sfbx::LayerMappingMode::ByPolygon;
		materialLayer.reference_mode = sfbx::LayerReferenceMode::Direct;
		
		geometry->addMaterialLayer(std::move(materialLayer));
	}
	
	*/
	// **Link geometry to model**
	parentModel->setGeometry(geometry);
}
