#include "import/Fbx.hpp"

#include "graphics/shading/MeshData.hpp"
#include "graphics/shading/MeshVertex.hpp"
#include "graphics/shading/MaterialProperties.hpp"

// Make sure the full FBX SDK header is included here
#include <fbxsdk.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace FbxUtil {

// Helper function to convert FbxMatrix to glm::mat4
glm::mat4 ToGlmMat4(const fbxsdk::FbxAMatrix& fbxMatrix) {
	glm::mat4 glmMat;
	memcpy(glm::value_ptr(glmMat), (const double*)fbxMatrix, sizeof(glm::mat4));
	return glmMat;
}

// Custom stream class to read an FBX file from a memory buffer.
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
			case FbxFile::eBegin:   newPos = pOffset; break;
			case FbxFile::eCurrent: newPos = static_cast<FbxInt64>(mPosition) + pOffset; break;
			case FbxFile::eEnd:     newPos = static_cast<FbxInt64>(mSize) + pOffset; break;
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

// Add this helper struct to Fbx.cpp
struct CompareVertexKey {
	bool operator()(const std::tuple<int, glm::vec3, glm::vec2>& a,
					const std::tuple<int, glm::vec3, glm::vec2>& b) const {
		// Perform a lexicographical comparison on the tuple elements
		if (std::get<0>(a) < std::get<0>(b)) return true;
		if (std::get<0>(b) < std::get<0>(a)) return false;
		
		// Element 0 is equal, compare element 1 (glm::vec3)
		const auto& v3_a = std::get<1>(a);
		const auto& v3_b = std::get<1>(b);
		if (v3_a.x < v3_b.x) return true;
		if (v3_b.x < v3_a.x) return false;
		if (v3_a.y < v3_b.y) return true;
		if (v3_b.y < v3_a.y) return false;
		if (v3_a.z < v3_b.z) return true;
		if (v3_b.z < v3_a.z) return false;
		
		// Elements 0 and 1 are equal, compare element 2 (glm::vec2)
		const auto& v2_a = std::get<2>(a);
		const auto& v2_b = std::get<2>(b);
		if (v2_a.x < v2_b.x) return true;
		if (v2_b.x < v2_a.x) return false;
		if (v2_a.y < v2_b.y) return true;
		if (v2_b.y < v2_a.y) return false;
		
		// All elements are equal
		return false;
	}
};

} // namespace FbxUtil

Fbx::Fbx() {
	// Create the FBX manager and settings objects.
	mFbxManager = FbxManager::Create();
	if (!mFbxManager) {
		std::cerr << "Error: Unable to create FBX Manager!" << std::endl;
		return;
	}
	
	FbxIOSettings* ios = FbxIOSettings::Create(mFbxManager, IOSROOT);
	mFbxManager->SetIOSettings(ios);
}

Fbx::~Fbx() {
	// Destroy the FBX manager, which will clean up the scene and other objects.
	if (mFbxManager) {
		mFbxManager->Destroy();
		mFbxManager = nullptr;
	}
}

void Fbx::LoadModel(const std::string& path) {
	if (!mFbxManager) return;
	
	fbxsdk::FbxImporter* importer = FbxImporter::Create(mFbxManager, "");
	
	if (!importer->Initialize(path.c_str(), -1, mFbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX importer for path: " << path << std::endl;
		std::cerr << "Error string: " << importer->GetStatus().GetErrorString() << std::endl;
		importer->Destroy();
		return;
	}
	
	mScene = FbxScene::Create(mFbxManager, "myScene");
	importer->Import(mScene);
	importer->Destroy();
	
	mFBXFilePath = path;
	
	if (!mScene) {
		std::cerr << "Error: Failed to import FBX scene from path: " << path << std::endl;
		return;
	}
	
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mScene);
	fbxsdk::FbxSystemUnit::m.ConvertScene(mScene);
	
	fbxsdk::FbxNode* rootNode = mScene->GetRootNode();
	if (rootNode) {
		ProcessNode(rootNode);
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	if (!mFbxManager) return;
	
	fbxsdk::FbxImporter* importer = FbxImporter::Create(mFbxManager, "");
	
	std::string str = data.str();
	FbxUtil::MemoryStreamFbxStream fbxStream(str.data(), str.size());
	
	if (!importer->Initialize(&fbxStream, nullptr, -1, mFbxManager->GetIOSettings())) {
		std::cerr << "Error: Failed to initialize FBX importer from stream." << std::endl;
		importer->Destroy();
		return;
	}
	
	mScene = FbxScene::Create(mFbxManager, "myStreamScene");
	importer->Import(mScene);
	importer->Destroy();
	
	if (!mScene) {
		std::cerr << "Error: Failed to import FBX scene from stream" << std::endl;
		return;
	}
	
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
	
	if (fbxsdk::FbxMesh* mesh = node->GetMesh()) {
		ProcessMesh(mesh, node);
	}
	
	for (int i = 0; i < node->GetChildCount(); ++i) {
		ProcessNode(node->GetChild(i));
	}
}
void Fbx::ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
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
	
	fbxsdk::FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
	glm::mat4 globalTransformGlm = FbxUtil::ToGlmMat4(globalTransform);
	glm::mat4 normalMatrix = glm::transpose(glm::inverse(globalTransformGlm));
	
	int polygonCount = mesh->GetPolygonCount();
	fbxsdk::FbxVector4* controlPoints = mesh->GetControlPoints();
	
	// [FIX] Use the custom comparator for the map's key
	std::map<std::tuple<int, glm::vec3, glm::vec2>, uint32_t, FbxUtil::CompareVertexKey> uniqueVertices;
	
	fbxsdk::FbxLayerElementMaterial* materialElement = mesh->GetLayer(0)->GetMaterials();
	int materialIndex = 0;
	
	for (int i = 0; i < polygonCount; ++i) {
		if (materialElement) {
			materialIndex = materialElement->GetIndexArray().GetAt(i);
		}
		
		for (int j = 0; j < 3; ++j) {
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			
			fbxsdk::FbxVector4 posFbx = controlPoints[controlPointIndex];
			glm::vec4 posGlm = {static_cast<float>(posFbx[0]), static_cast<float>(posFbx[1]), static_cast<float>(posFbx[2]), 1.0f};
			posGlm = globalTransformGlm * posGlm;
			
			fbxsdk::FbxVector4 normalFbx;
			mesh->GetPolygonVertexNormal(i, j, normalFbx);
			normalFbx.Normalize();
			glm::vec4 normalGlm = {static_cast<float>(normalFbx[0]), static_cast<float>(normalFbx[1]), static_cast<float>(normalFbx[2]), 0.0f};
			normalGlm = normalMatrix * normalGlm;
			glm::vec3 finalNormal = glm::normalize(glm::vec3(normalGlm));
			
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
			
			std::tuple<int, glm::vec3, glm::vec2> vertexKey(controlPointIndex, finalNormal, uvGlm);
			
			if (uniqueVertices.find(vertexKey) == uniqueVertices.end()) {
				auto vertex = std::make_unique<MeshVertex>();
				vertex->set_position({posGlm.x, posGlm.y, posGlm.z});
				vertex->set_normal(finalNormal);
				vertex->set_texture_coords1(uvGlm);
				vertex->set_material_id(materialIndex);
				
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
				indices.push_back(uniqueVertices[vertexKey]);
			}
		}
	}
	
	// ... The rest of the function remains the same
	int materialCount = node->GetMaterialCount();
	std::vector<std::shared_ptr<MaterialProperties>> meshMaterials;
	meshMaterials.resize(std::max(1, materialCount));
	
	for (int i = 0; i < materialCount; ++i) {
		fbxsdk::FbxSurfaceMaterial* material = node->GetMaterial(i);
		auto matPtr = std::make_shared<MaterialProperties>();
		
		if (auto* lambert = FbxCast<FbxSurfaceLambert>(material)) {
			matPtr->mAmbient = {(float)lambert->Ambient.Get()[0], (float)lambert->Ambient.Get()[1], (float)lambert->Ambient.Get()[2], 1.0f};
			matPtr->mDiffuse = {(float)lambert->Diffuse.Get()[0], (float)lambert->Diffuse.Get()[1], (float)lambert->Diffuse.Get()[2], 1.0f};
			matPtr->mOpacity = 1.0f - (float)lambert->TransparencyFactor.Get();
		}
		if (auto* phong = FbxCast<FbxSurfacePhong>(material)) {
			matPtr->mSpecular = {(float)phong->Specular.Get()[0], (float)phong->Specular.Get()[1], (float)phong->Specular.Get()[2], 1.0f};
			matPtr->mShininess = (float)phong->Shininess.Get();
		}
		
		fbxsdk::FbxProperty diffuseProp = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sDiffuse);
		if (diffuseProp.IsValid() && diffuseProp.GetSrcObjectCount<fbxsdk::FbxFileTexture>() > 0) {
			if (fbxsdk::FbxFileTexture* texture = diffuseProp.GetSrcObject<fbxsdk::FbxFileTexture>(0)) {
				std::string texturePath = texture->GetFileName();
				
				std::filesystem::path finalPath(texturePath);
				if (!std::filesystem::exists(finalPath)) {
					finalPath = std::filesystem::path(mFBXFilePath).parent_path() / finalPath.filename();
				}
				
				if (std::filesystem::exists(finalPath)) {
					std::ifstream file(finalPath, std::ios::binary);
					if (file) {
						std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
						if (!buffer.empty()) {
							// matPtr->mTextureDiffuse = std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
							matPtr->mHasDiffuseTexture = true;
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
		meshMaterials.push_back(std::make_shared<MaterialProperties>());
	}
	
	resultMesh->get_material_properties() = meshMaterials;
	mMaterialProperties.push_back(meshMaterials);
	
	ProcessBones(mesh);
}


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
