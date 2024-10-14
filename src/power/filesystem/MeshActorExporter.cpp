#include "MeshActorExporter.hpp"
#include "MeshDeserializer.hpp"
#include "VectorConversion.hpp"

#include <fbxsdk.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <iostream>
#include <thread>       // For std::thread
#include <exception>    // For std::exception
#include <functional>   // For std::function

namespace ExporterUtil {

// Extract Euler angles (in degrees) from a quaternion rotation
glm::vec3 ExtractEulerAngles(const glm::quat& rotation) {
	glm::vec3 euler = glm::eulerAngles(rotation);
	// Convert from radians to degrees
	return glm::degrees(euler);
}

// Decompose a transformation matrix into translation, rotation, and scale
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4& transform) {
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return { translation, rotation, scale };
}

// Convert glm::mat4 to FbxAMatrix
FbxAMatrix GlmMatToFbxAMatrix(const glm::mat4& from) {
	FbxAMatrix to;
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			to[row][column] = from[column][row];
		}
	}
	return to;
}

// Convert glm::vec3 to FbxDouble3
FbxDouble3 GlmVec3ToFbxDouble3(const glm::vec3& vec) {
	return FbxDouble3(vec.x, vec.y, vec.z);
}

}  // namespace ExporterUtil

// Constructor
MeshActorExporter::MeshActorExporter()
{
	mMeshDeserializer = std::make_unique<MeshDeserializer>();
	// Initialize other members if necessary
}

// Destructor
MeshActorExporter::~MeshActorExporter() {
	// Resources are managed by smart pointers, so no manual cleanup is needed
}

// Common function to set up the scene (used by both exportToFile and exportToStream)
bool MeshActorExporter::setupScene(CompressedSerialization::Deserializer& deserializer,
								   const std::string& sourcePath,
								   FbxManager* fbxManager,
								   FbxScene* scene) {
	mMeshModels.clear();
	
	// Step 1: Deserialize or transfer data from CompressedMeshActor to internal structures
	mMeshDeserializer->load_mesh(deserializer, sourcePath);
	// Retrieve the deserialized meshes
	mMeshes = std::move(mMeshDeserializer->get_meshes());
	
	// Step 2: Create Materials
	std::map<int, FbxSurfacePhong*> materialMap; // Map textureID to Material
	if (!mMeshes.empty()) {
		// Upcast shared_ptr<MaterialProperties> to shared_ptr<SerializableMaterialProperties>
		std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
		serializableMaterials.reserve(mMeshes[0]->get_material_properties().size());
		
		for (const auto& mat : mMeshes[0]->get_material_properties()) {
			// Ensure that mat is indeed a shared_ptr<SerializableMaterialProperties>
			serializableMaterials.emplace_back(std::make_shared<SerializableMaterialProperties>(*mat));
		}
		
		createMaterials(fbxManager, scene, serializableMaterials, materialMap);
	}
	
	// Step 3: Create Meshes and Models
	FbxNode* rootNode = scene->GetRootNode();
	
	for (size_t i = 0; i < mMeshes.size(); ++i) {
		auto& meshData = *mMeshes[i];
		
		FbxMesh* fbxMesh = FbxMesh::Create(scene, ("Mesh_" + std::to_string(i)).c_str());
		FbxNode* meshNode = FbxNode::Create(scene, ("Mesh_Node_" + std::to_string(i)).c_str());
		meshNode->SetNodeAttribute(fbxMesh);
		rootNode->AddChild(meshNode);
		
		createMesh(meshData, materialMap, fbxMesh, meshNode);
		
		// Store the mesh node for future use
		mMeshModels.push_back(meshNode);
	}
	
	// Step 4: Build skeleton
	auto skeleton = mMeshDeserializer->get_skeleton();
	
	if (skeleton != std::nullopt) {
		// Get the actual Skeleton object
		auto& skeletonData = *skeleton;
		
		// Map to store bone index to FbxNode
		std::map<int, FbxNode*> boneNodes;
		
		// Step 1: Create FbxNode nodes for each bone
		for (int boneIndex = 0; boneIndex < skeletonData.num_bones(); ++boneIndex) {
			std::string boneName = skeletonData.get_bone(boneIndex).name;
			int parentIndex = skeletonData.get_bone(boneIndex).parent_index;
			
			// Create a new Skeleton node for the bone
			FbxSkeleton* fbxSkeleton = FbxSkeleton::Create(scene, boneName.c_str());
			fbxSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
			
			FbxNode* boneNode = FbxNode::Create(scene, boneName.c_str());
			boneNode->SetNodeAttribute(fbxSkeleton);
			
			// Step 2: Set bone's local transformations
			glm::mat4 poseMatrix = skeletonData.get_bone(boneIndex).bindpose;
			
			glm::vec3 translation, scale;
			glm::quat rotation;
			std::tie(translation, rotation, scale) = ExporterUtil::DecomposeTransform(poseMatrix);
			
			boneNode->LclTranslation.Set(ExporterUtil::GlmVec3ToFbxDouble3(translation));
			glm::vec3 eulerAngles = ExporterUtil::ExtractEulerAngles(rotation);
			boneNode->LclRotation.Set(ExporterUtil::GlmVec3ToFbxDouble3(eulerAngles));
			boneNode->LclScaling.Set(ExporterUtil::GlmVec3ToFbxDouble3(scale));
			
			// Store the bone node in the map
			boneNodes[boneIndex] = boneNode;
		}
		
		// Step 3: Establish parent-child relationships
		for (int boneIndex = 0; boneIndex < skeletonData.num_bones(); ++boneIndex) {
			int parentIndex = skeletonData.get_bone(boneIndex).parent_index;
			FbxNode* boneNode = boneNodes[boneIndex];
			
			if (parentIndex == -1) {
				// Root bone: attach to the root node
				rootNode->AddChild(boneNode);
			} else {
				// Child bone: attach to its parent bone
				FbxNode* parentNode = boneNodes[parentIndex];
				parentNode->AddChild(boneNode);
			}
		}
		
		// Step 4: Associate the skeleton with the mesh via skinning
		for (size_t i = 0; i < mMeshes.size(); ++i) {
			auto& meshData = *mMeshes[i];
			FbxNode* meshNode = mMeshModels[i];
			FbxMesh* fbxMesh = meshNode->GetMesh();
			
			if (!fbxMesh) {
				std::cerr << "Error: Mesh node does not have a mesh attribute." << std::endl;
				continue;
			}
			
			// Create a Skin deformer and add it to the mesh
			FbxSkin* skin = FbxSkin::Create(scene, ("Skin_" + std::string{ meshNode->GetName() }).c_str());
			fbxMesh->AddDeformer(skin);
			
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
					
					if (boneID == -1) {
						continue;
					}
					
					boneWeights[boneID].emplace_back(static_cast<int>(vi), weight);
				}
			}
			
			// Create clusters for each bone and associate them with the skin deformer
			for (const auto& [boneID, weightData] : boneWeights) {
				FbxCluster* cluster = FbxCluster::Create(scene, ("Cluster_" + std::to_string(boneID)).c_str());
				skin->AddCluster(cluster);
				
				// Set the link node to the corresponding bone node
				FbxNode* boneNode = boneNodes[boneID];
				cluster->SetLink(boneNode);
				
				// Set the link mode
				cluster->SetLinkMode(FbxCluster::eNormalize);
				
				// Add control point indices and weights
				for (const auto& [vertexIndex, weight] : weightData) {
					cluster->AddControlPointIndex(vertexIndex, static_cast<double>(weight));
				}
				
				// Set the transform and transform link matrices
				FbxAMatrix meshTransformMatrix = meshNode->EvaluateGlobalTransform();
				cluster->SetTransformMatrix(meshTransformMatrix);
				
				FbxAMatrix boneTransformMatrix = boneNode->EvaluateGlobalTransform();
				cluster->SetTransformLinkMatrix(boneTransformMatrix);
			}
		}
	}
	
	return true;
}

// Synchronous exportToFile implementation
bool MeshActorExporter::exportToFile(CompressedSerialization::Deserializer& deserializer,
									 const std::string& sourcePath,
									 const std::string& exportPath) {
	
	// Initialize the FBX Manager and Scene
	FbxManager* fbxManager = FbxManager::Create();
	if (!fbxManager) {
		std::cerr << "Error: Unable to create FBX Manager!" << std::endl;
		return false;
	}
	
	FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
	
	fbxManager->SetIOSettings(ios);
	
	FbxScene* scene = FbxScene::Create(fbxManager, "MyScene");
	
	FbxAxisSystem directXAxisSys(FbxAxisSystem::EUpVector::eZAxis,
								 FbxAxisSystem::EFrontVector::eParityOdd,
								 FbxAxisSystem::eRightHanded);

	scene->GetGlobalSettings().SetAxisSystem(directXAxisSys);
	
	if (!scene) {
		std::cerr << "Error: Unable to create FBX scene!" << std::endl;
		fbxManager->Destroy();
		return false;
	}
	
	if (!setupScene(deserializer, sourcePath, fbxManager, scene)) {
		fbxManager->Destroy();
		return false;
	}
	
	// Step 5: Export the Scene to a file
	FbxExporter* exporter = FbxExporter::Create(fbxManager, "");
	
	if (!exporter->Initialize(exportPath.c_str(), -1, fbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX exporter." << std::endl;
		exporter->Destroy();
		fbxManager->Destroy();
		return false;
	}
		
	if (!exporter->Export(scene)) {
		std::cerr << "Error: Failed to export FBX scene." << std::endl;
		exporter->Destroy();
		fbxManager->Destroy();
		return false;
	}
	
	exporter->Destroy();
	fbxManager->Destroy();
	
	std::cout << "Successfully exported actor to " << exportPath << std::endl;
	return true;
}

// Synchronous exportToStream implementation
bool MeshActorExporter::exportToStream(CompressedSerialization::Deserializer& deserializer,
									   const std::string& sourcePath,
									   std::ostream& outStream) {
	
	// Initialize the FBX Manager and Scene
	FbxManager* fbxManager = FbxManager::Create();
	if (!fbxManager) {
		std::cerr << "Error: Unable to create FBX Manager!" << std::endl;
		return false;
	}
	
	FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
	fbxManager->SetIOSettings(ios);
	
	FbxScene* scene = FbxScene::Create(fbxManager, "MyScene");
	
	FbxAxisSystem directXAxisSys(FbxAxisSystem::EUpVector::eZAxis,
								 FbxAxisSystem::EFrontVector::eParityOdd,
								 FbxAxisSystem::eRightHanded);
	
	scene->GetGlobalSettings().SetAxisSystem(directXAxisSys);

	if (!scene) {
		std::cerr << "Error: Unable to create FBX scene!" << std::endl;
		fbxManager->Destroy();
		return false;
	}
	
	if (!setupScene(deserializer, sourcePath, fbxManager, scene)) {
		fbxManager->Destroy();
		return false;
	}
	
	// Create an exporter
	FbxExporter* exporter = FbxExporter::Create(fbxManager, "");
	
	// Implement a custom stream class that uses the provided std::ostream
	class OStreamWrapper : public FbxStream {
	public:
		OStreamWrapper(std::ostream& stream) : mStream(stream) {}
		virtual ~OStreamWrapper() {}
		
		virtual EState GetState() override { return mStream.good() ? eOpen : eClosed; }
		virtual bool Open(void* /*pStreamData*/) override {
			return true;
		}
		virtual bool Close() override { return true; }
		virtual bool Flush() override { mStream.flush(); return true; }
		virtual size_t Write(const void* pData, FbxUInt64 pSize) override {
			mStream.write(static_cast<const char*>(pData), pSize);
			return pSize;
		}
		virtual size_t Read(void* /*pData*/, FbxUInt64 /*pSize*/) const override { return 0; }
		virtual int GetReaderID() const override { return -1; }
		virtual int GetWriterID() const override { return 0; }
		virtual void Seek(const FbxInt64& pOffset, const FbxFile::ESeekPos& pSeekPos) override {
			std::ios_base::seekdir dir;
			switch (pSeekPos) {
				case FbxFile::eBegin:
					dir = std::ios_base::beg;
					break;
				case FbxFile::eCurrent:
					dir = std::ios_base::cur;
					break;
				case FbxFile::eEnd:
					dir = std::ios_base::end;
					break;
				default:
					dir = std::ios_base::beg;
					break;
			}
			mStream.seekp(static_cast<std::streamoff>(pOffset), dir);
		}
		virtual FbxInt64 GetPosition() const override {
			return static_cast<FbxInt64>(mStream.tellp());
		}
		virtual void SetPosition(FbxInt64 pPosition) override {
			mStream.seekp(static_cast<std::streamoff>(pPosition), std::ios_base::beg);
		}
		virtual int GetError() const override { return 0; }
		virtual void ClearError() override {}
	private:
		std::ostream& mStream;
	};
	
	OStreamWrapper customStream(outStream);
	
	// Initialize the exporter with the custom stream
	if (!exporter->Initialize(&customStream, nullptr, -1, fbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX exporter with custom stream." << std::endl;
		exporter->Destroy();
		fbxManager->Destroy();
		return false;
	}
	
	if (!exporter->Export(scene)) {
		std::cerr << "Error: Failed to export FBX scene." << std::endl;
		exporter->Destroy();
		fbxManager->Destroy();
		return false;
	}

	exporter->Destroy();
	fbxManager->Destroy();
	
	std::cout << "Successfully exported actor to stream." << std::endl;
	return true;
}

// Helper function to create materials
void MeshActorExporter::createMaterials(FbxManager* fbxManager,
										FbxScene* scene,
										const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials,
										std::map<int, FbxSurfacePhong*>& materialMap)
{
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto& matProps = materials[i];
		FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, ("Material_" + std::to_string(i)).c_str());
		
		// Set material properties
		material->Ambient.Set(ExporterUtil::GlmVec3ToFbxDouble3(glm::vec3(matProps->mAmbient)));
		material->Diffuse.Set(ExporterUtil::GlmVec3ToFbxDouble3(glm::vec3(matProps->mDiffuse)));
		material->Specular.Set(ExporterUtil::GlmVec3ToFbxDouble3(glm::vec3(matProps->mSpecular)));
		material->Shininess.Set(matProps->mShininess);
		material->TransparencyFactor.Set(1.0 - matProps->mOpacity);
		
		// Handle textures
		if (matProps->mHasDiffuseTexture) {
			FbxFileTexture* texture = FbxFileTexture::Create(scene, ("Texture_" + std::to_string(i)).c_str());
			
			// Save texture data to a temporary file or handle embedding
			// For simplicity, we'll skip embedding textures in this example
			
			// Assign the texture to the material
			material->Diffuse.ConnectSrcObject(texture);
		}
		
		materialMap[i] = material;
	}
}

// Helper function to create mesh geometry
void MeshActorExporter::createMesh(
								   MeshData& meshData,
								   const std::map<int, FbxSurfacePhong*>& materialMap,
								   FbxMesh* fbxMesh,
								   FbxNode* meshNode)
{
	// Set control points (vertices)
	int vertexCount = static_cast<int>(meshData.get_vertices().size());
	fbxMesh->InitControlPoints(vertexCount);
	
	for (int i = 0; i < vertexCount; ++i) {
		auto& vertex = *meshData.get_vertices()[i];
		fbxMesh->SetControlPointAt(FbxVector4(vertex.get_position().x, vertex.get_position().y, vertex.get_position().z), i);
	}
	
	// Create layer elements
	FbxLayer* layer = fbxMesh->GetLayer(0);
	if (!layer) {
		fbxMesh->CreateLayer();
		layer = fbxMesh->GetLayer(0);
	}
	
	// Normals
	FbxLayerElementNormal* layerElementNormal = FbxLayerElementNormal::Create(fbxMesh, "");
	layerElementNormal->SetMappingMode(FbxLayerElement::eByControlPoint);
	layerElementNormal->SetReferenceMode(FbxLayerElement::eDirect);
	for (const auto& vertexPtr : meshData.get_vertices()) {
		auto& vertex = *vertexPtr;
		layerElementNormal->GetDirectArray().Add(FbxVector4(vertex.get_normal().x, vertex.get_normal().y, vertex.get_normal().z));
	}
	layer->SetNormals(layerElementNormal);
	
	// UVs
	FbxLayerElementUV* layerElementUV = FbxLayerElementUV::Create(fbxMesh, "UVChannel_1");
	layerElementUV->SetMappingMode(FbxLayerElement::eByControlPoint);
	layerElementUV->SetReferenceMode(FbxLayerElement::eDirect);
	for (const auto& vertexPtr : meshData.get_vertices()) {
		auto& vertex = *vertexPtr;
		layerElementUV->GetDirectArray().Add(FbxVector2(vertex.get_tex_coords1().x, vertex.get_tex_coords1().y));
	}
	layer->SetUVs(layerElementUV, FbxLayerElement::eTextureDiffuse);
	
	// Colors
	FbxLayerElementVertexColor* layerElementColor = FbxLayerElementVertexColor::Create(fbxMesh, "");
	layerElementColor->SetMappingMode(FbxLayerElement::eByControlPoint);
	layerElementColor->SetReferenceMode(FbxLayerElement::eDirect);
	for (const auto& vertexPtr : meshData.get_vertices()) {
		auto& vertex = *vertexPtr;
		layerElementColor->GetDirectArray().Add(FbxColor(vertex.get_color().r, vertex.get_color().g, vertex.get_color().b, vertex.get_color().a));
	}
	layer->SetVertexColors(layerElementColor);
	
	// Create material layer element
	FbxLayerElementMaterial* layerElementMaterial = FbxLayerElementMaterial::Create(fbxMesh, "");
	layerElementMaterial->SetMappingMode(FbxLayerElement::eByPolygon);
	layerElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	layer->SetMaterials(layerElementMaterial);
	
	// Set indices (polygons)
	auto& indices = meshData.get_indices();
	int polygonCount = static_cast<int>(indices.size() / 3);
	for (int i = 0; i < polygonCount; ++i) {
		fbxMesh->BeginPolygon();
		
		// Assign material to the polygon
		int materialIndex = meshData.get_vertices()[indices[i * 3]]->get_material_id();
		// Add the material index to the layer element
		layerElementMaterial->GetIndexArray().Add(materialIndex);
		
		fbxMesh->AddPolygon(indices[i * 3]);
		fbxMesh->AddPolygon(indices[i * 3 + 1]);
		fbxMesh->AddPolygon(indices[i * 3 + 2]);
		
		fbxMesh->EndPolygon();
	}
	
	// Attach materials to the mesh node
	for (const auto& [index, material] : materialMap) {
		meshNode->AddMaterial(material);
	}
}

// Asynchronous exportToFile implementation
void MeshActorExporter::exportToFileAsync(CompressedSerialization::Deserializer& deserializer,
										  const std::string& sourcePath,
										  const std::string& exportPath,
										  std::function<void(bool success)> callback) {
	std::thread([this, &deserializer, sourcePath, exportPath, callback]() {
		bool success = this->exportToFile(deserializer, sourcePath, exportPath);
		if (callback) {
			callback(success);
		}
	}).detach();
}

// Asynchronous exportToStream implementation
void MeshActorExporter::exportToStreamAsync(std::unique_ptr<CompressedSerialization::Deserializer> deserializerPtr,
											const std::string& sourcePath,
											std::ostream& outStream,
											std::function<void(bool success)> callback) {
	std::thread([this, deserializer = std::move(deserializerPtr), sourcePath, &outStream, callback]() mutable {
		bool success = this->exportToStream(*deserializer, sourcePath, outStream);
		if (callback) {
			callback(success);
		}
	}).detach();
}
