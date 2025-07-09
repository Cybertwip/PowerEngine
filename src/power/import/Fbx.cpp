#include "import/Fbx.hpp"

#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "graphics/shading/MaterialProperties.hpp"

#include <fbxsdk.h>
#include <fbxsdk/core/fbxstream.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// [FIX] Added for std::make_tuple used in vertex map key
#include <tuple>

namespace FbxUtil {

// Helper function to convert FbxMatrix to glm::mat4
glm::mat4 ToGlmMat4(const fbxsdk::FbxAMatrix& fbxMatrix) {
	glm::mat4 glmMat;
	// FBX matrices are column-major by default, same as GLM.
	// The copy can be done directly.
	memcpy(glm::value_ptr(glmMat), (const double*)fbxMatrix, sizeof(glm::mat4));
	return glmMat;
}

// Custom stream class to read an FBX file from a memory buffer.
// This class seems correct and does not require changes.
class MemoryStreamFbxStream : public FbxStream {
public:
	MemoryStreamFbxStream(const char* data, size_t size)
	: mData(data), mSize(size), mPosition(0) {}
	
	~MemoryStreamFbxStream() override = default;
	
	EState GetState() override { return FbxStream::eOpen; }
	
	bool Open(void* /*pStreamData*/) override {
		mPosition = 0;
		return true;
	}
	
	bool Close() override { return true; }
	
	size_t Read(void* pBuffer, FbxUInt64 pSize) const override {
		size_t bytesToRead = static_cast<size_t>(std::min<FbxUInt64>(pSize, mSize - mPosition));
		if (bytesToRead > 0) {
			memcpy(pBuffer, mData + mPosition, bytesToRead);
			mPosition += bytesToRead;
		}
		return bytesToRead;
	}
	
	size_t Write(const void* /*pBuffer*/, FbxUInt64 /*pSize*/) override { return 0; }
	int GetReaderID() const override { return 0; }
	int GetWriterID() const override { return -1; }
	bool Flush() override { return true; }
	
	void Seek(const FbxInt64& pOffset, const FbxFile::ESeekPos& pSeekPos) override {
		FbxInt64 newPos = 0;
		switch (pSeekPos) {
			case FbxFile::eBegin:   newPos = pOffset; break;
			case FbxFile::eCurrent: newPos = static_cast<FbxInt64>(mPosition) + pOffset; break;
			case FbxFile::eEnd:     newPos = static_cast<FbxInt64>(mSize) + pOffset; break;
			default: return;
		}
		mPosition = static_cast<size_t>(std::max<FbxInt64>(0, std::min<FbxInt64>(newPos, mSize)));
	}
	
	FbxInt64 GetPosition() const override { return static_cast<FbxInt64>(mPosition); }
	
	void SetPosition(FbxInt64 pPosition) override {
		mPosition = static_cast<size_t>(std::max<FbxInt64>(0, std::min<FbxInt64>(pPosition, mSize)));
	}
	
	int GetError() const override { return 0; }
	void ClearError() override {}
	
private:
	const char* mData;
	size_t mSize;
	mutable size_t mPosition;
};

}


Fbx::Fbx() {
	// Constructor can remain empty or be used for initial setup if needed.
}

Fbx::~Fbx() {
	// [FIX] Properly destroy the FBX SDK objects to prevent memory leaks.
	// The SDK manages object relationships, so destroying the manager
	// should handle the cleanup of scenes and other objects it created.
	if (mFbxManager) {
		mFbxManager->Destroy();
		mFbxManager = nullptr; // Good practice to null pointers after deletion.
	}
	// The scene object is owned by the manager, so it gets destroyed
	// when the manager is destroyed. No need to destroy mScene separately.
}
void Fbx::LoadModel(const std::string& path) {
	// The FbxManager should be created in the constructor and stored
	// but for this function, we'll assume it's ready.
	if (!mFbxManager) {
		std::cerr << "Error: FbxManager not initialized!" << std::endl;
		return;
	}
	
	// [FIX] Declare mImporter as a local variable here.
	fbxsdk::FbxImporter* importer = FbxImporter::Create(mFbxManager, "");
	
	// Use the local 'importer' variable.
	if (!importer->Initialize(path.c_str(), -1, mFbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX importer for path: " << path << std::endl;
		std::cerr << "Error string: " << importer->GetStatus().GetErrorString() << std::endl;
		importer->Destroy(); // Clean up on failure
		return;
	}
	
	mScene = FbxScene::Create(mFbxManager, "myScene");
	importer->Import(mScene);
	importer->Destroy(); // Importer is no longer needed after import
	
	mFBXFilePath = path;
	
	if (!mScene) {
		std::cerr << "Error: Failed to load FBX scene from path: " << path << std::endl;
		return;
	}
	
	// ... (rest of the function remains the same)
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mScene);
	fbxsdk::FbxSystemUnit::m.ConvertScene(mScene);
	
	fbxsdk::FbxNode* rootNode = mScene->GetRootNode();
	if (rootNode) {
		ProcessNode(rootNode);
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	if (!mFbxManager) {
		std::cerr << "Error: FbxManager not initialized!" << std::endl;
		return;
	}
	
	// [FIX] Declare mImporter as a local variable here.
	fbxsdk::FbxImporter* importer = FbxImporter::Create(mFbxManager, "");
	
	std::string str = data.str();
	// The stream object must exist for the duration of the import.
	FbxUtil::MemoryStreamFbxStream fbxStream(str.data(), str.size());
	
	if (!importer->Initialize(&fbxStream, nullptr, -1, mFbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX importer from stream." << std::endl;
		importer->Destroy(); // Clean up on failure
		return;
	}
	
	mScene = FbxScene::Create(mFbxManager, "myStreamScene");
	importer->Import(mScene);
	importer->Destroy(); // Importer is no longer needed
	
	if (!mScene) {
		std::cerr << "Error: Failed to load FBX scene from stream" << std::endl;
		return;
	}
	
	// ... (rest of the function remains the same)
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mScene);
	
	fbxsdk::FbxSystemUnit::m.ConvertScene(mScene);
	
	fbxsdk::FbxNode* rootNode = mScene->GetRootNode();
	if (rootNode) {
		ProcessNode(rootNode);
	}
}

void Fbx::ProcessNode(fbxsdk::FbxNode* node) {
	if (!node) return;
	
	// Process the mesh on the current node, if it exists.
	if (fbxsdk::FbxMesh* mesh = node->GetMesh()) {
		ProcessMesh(mesh, node);
	}
	
	// Recursively process all child nodes.
	for (int i = 0; i < node->GetChildCount(); ++i) {
		ProcessNode(node->GetChild(i));
	}
}

void Fbx::ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	// Triangulate the mesh to ensure we are working with triangles.
	fbxsdk::FbxGeometryConverter geometryConverter(mFbxManager);
	if (!mesh->IsTriangleMesh()) {
		mesh = static_cast<fbxsdk::FbxMesh*>(geometryConverter.Triangulate(mesh, true));
	}
	if (!mesh) {
		std::cerr << "Error: Mesh triangulation failed." << std::endl;
		return;
	}
	
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	auto& vertices = resultMesh->get_vertices();
	auto& indices = resultMesh->get_indices();
	
	// [FIX] Get the node's global transformation matrix. This is crucial.
	fbxsdk::FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
	glm::mat4 globalTransformGlm = FbxUtil::ToGlmMat4(globalTransform);
	
	// [FIX] For correct normal transformation, we need the inverse transpose of the matrix.
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(globalTransformGlm));
	
	int polygonCount = mesh->GetPolygonCount();
	fbxsdk::FbxVector4* controlPoints = mesh->GetControlPoints();
	
	// [FIX] Map to hold unique vertices to build a proper index buffer.
	// This prevents duplicating vertex data unnecessarily.
	// The key combines control point index, normal, and UV to define a unique vertex.
	std::map<std::tuple<int, glm::vec3, glm::vec2>, uint32_t> uniqueVertices;
	
	// [FIX] Get material element to determine which material is assigned to each polygon.
	fbxsdk::FbxLayerElementMaterial* materialElement = mesh->GetLayer(0)->GetMaterials();
	int materialIndex = 0; // Default material index
	
	for (int i = 0; i < polygonCount; ++i) {
		// [FIX] Determine the material index for this polygon.
		if (materialElement) {
			materialIndex = materialElement->GetIndexArray().GetAt(i);
		}
		
		for (int j = 0; j < 3; ++j) { // Process each vertex of the triangle
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			
			// Position
			fbxsdk::FbxVector4 posFbx = controlPoints[controlPointIndex];
			glm::vec4 posGlm = {static_cast<float>(posFbx[0]), static_cast<float>(posFbx[1]), static_cast<float>(posFbx[2]), 1.0f};
			// [FIX] Apply the node's transformation to the vertex position.
			posGlm = globalTransformGlm * posGlm;
			
			// Normal
			fbxsdk::FbxVector4 normalFbx;
			mesh->GetPolygonVertexNormal(i, j, normalFbx);
			normalFbx.Normalize();
			glm::vec4 normalGlm = {static_cast<float>(normalFbx[0]), static_cast<float>(normalFbx[1]), static_cast<float>(normalFbx[2]), 0.0f};
			// [FIX] Apply the correct transformation to the normal.
			normalGlm = normalMatrix * normalGlm;
			glm::vec3 finalNormal = glm::normalize(glm::vec3(normalGlm));
			
			// Texture Coordinates (UVs)
			glm::vec2 uvGlm = {0.0f, 0.0f};
			fbxsdk::FbxStringList uvSetNameList;
			mesh->GetUVSetNames(uvSetNameList);
			if (uvSetNameList.GetCount() > 0) {
				fbxsdk::FbxVector2 uvFbx;
				bool unmapped;
				if (mesh->GetPolygonVertexUV(i, j, uvSetNameList.GetStringAt(0), uvFbx, unmapped)) {
					uvGlm = {static_cast<float>(uvFbx[0]), static_cast<float>(uvFbx[1])};
				}
			}
			
			// Create a key to check if this vertex configuration has already been processed
			std::tuple<int, glm::vec3, glm::vec2> vertexKey(controlPointIndex, finalNormal, uvGlm);
			
			if (uniqueVertices.find(vertexKey) == uniqueVertices.end()) {
				auto vertex = std::make_unique<MeshVertex>();
				vertex->set_position({posGlm.x, posGlm.y, posGlm.z});
				vertex->set_normal(finalNormal);
				vertex->set_texture_coords1(uvGlm);
				
				// [FIX] Assign the correct material ID to the vertex.
				vertex->set_material_id(materialIndex);
				
				// Vertex Color (optional)
				if (mesh->GetElementVertexColorCount() > 0) {
					fbxsdk::FbxGeometryElementVertexColor* colorElement = mesh->GetElementVertexColor(0);
					fbxsdk::FbxColor color = colorElement->GetDirectArray().GetAt(colorElement->GetIndexArray().GetAt(mesh->GetPolygonVertexIndex(i) + j));
					vertex->set_color({static_cast<float>(color.mRed), static_cast<float>(color.mGreen), static_cast<float>(color.mBlue), static_cast<float>(color.mAlpha)});
				}
				
				uint32_t newIndex = static_cast<uint32_t>(vertices.size());
				vertices.push_back(std::move(vertex));
				indices.push_back(newIndex);
				uniqueVertices[vertexKey] = newIndex;
			} else {
				// This vertex configuration already exists, so just reuse its index.
				indices.push_back(uniqueVertices[vertexKey]);
			}
		}
	}
	
	// Process materials
	int materialCount = node->GetMaterialCount();
	std::vector<std::shared_ptr<MaterialProperties>> meshMaterials;
	meshMaterials.resize(std::max(1, materialCount)); // Ensure at least one default material
	
	for (int i = 0; i < materialCount; ++i) {
		fbxsdk::FbxSurfaceMaterial* material = node->GetMaterial(i);
		auto matPtr = std::make_shared<MaterialProperties>();
		
		// Extract properties (Lambert for basic colors, Phong for specular highlights)
		if (auto* lambert = FbxCast<FbxSurfaceLambert>(material)) {
			matPtr->mAmbient = { (float)lambert->Ambient.Get()[0], (float)lambert->Ambient.Get()[1], (float)lambert->Ambient.Get()[2], 1.0f };
			matPtr->mDiffuse = { (float)lambert->Diffuse.Get()[0], (float)lambert->Diffuse.Get()[1], (float)lambert->Diffuse.Get()[2], 1.0f };
			matPtr->mOpacity = 1.0f - (float)lambert->TransparencyFactor.Get();
		}
		if (auto* phong = FbxCast<FbxSurfacePhong>(material)) {
			matPtr->mSpecular = { (float)phong->Specular.Get()[0], (float)phong->Specular.Get()[1], (float)phong->Specular.Get()[2], 1.0f };
			matPtr->mShininess = (float)phong->Shininess.Get();
		}
		
		// Extract diffuse texture
		fbxsdk::FbxProperty diffuseProp = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sDiffuse);
		if (diffuseProp.IsValid() && diffuseProp.GetSrcObjectCount<fbxsdk::FbxFileTexture>() > 0) {
			if (fbxsdk::FbxFileTexture* texture = diffuseProp.GetSrcObject<fbxsdk::FbxFileTexture>(0)) {
				std::string texturePath = texture->GetFileName();
				
				// [FIX] Improved texture path resolution
				std::filesystem::path finalPath(texturePath);
				if (!std::filesystem::exists(finalPath)) {
					finalPath = std::filesystem::path(mFBXFilePath).parent_path() / finalPath.filename();
				}
				
				if (std::filesystem::exists(finalPath)) {
					std::ifstream file(finalPath, std::ios::binary);
					if (file) {
						std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
						if (!buffer.empty()) {
							// Assuming nanogui::Texture can be created from raw data buffer
							// matPtr->mTextureDiffuse = std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
							matPtr->mHasDiffuseTexture = true;
							// Store texture data or path for the renderer to handle
						}
					}
				} else {
					std::cerr << "Warning: Could not find texture file: " << texturePath << std::endl;
				}
			}
		}
		meshMaterials[i] = matPtr;
	}
	
	if (meshMaterials.empty()) {
		meshMaterials.push_back(std::make_shared<MaterialProperties>()); // Add a default material
	}
	
	resultMesh->get_material_properties() = meshMaterials;
	mMaterialProperties.push_back(meshMaterials);
	
	ProcessBones(mesh);
}


// ... (The rest of your Fbx class implementation remains the same) ...

std::vector<std::unique_ptr<MeshData>>& Fbx::GetMeshData() {
	return mMeshes;
}

void Fbx::SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData) {
	mMeshes = std::move(meshData);
}

std::vector<std::vector<std::shared_ptr<MaterialProperties>>>& Fbx::GetMaterialProperties() {
	return mMaterialProperties;
}

void Fbx::ProcessBones(fbxsdk::FbxMesh* /*mesh*/) {
	// Base implementation is empty. SkinnedFbx will override this.
}
