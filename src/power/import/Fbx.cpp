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

namespace FbxUtil {

// Helper function to convert FbxMatrix to glm::mat4
glm::mat4 ToGlmMat4(const fbxsdk::FbxAMatrix& fbxMatrix) {
	glm::mat4 glmMat;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			// FBX matrices are column-major, same as GLM.
			// However, the Get method is (row, col).
			glmMat[i][j] = static_cast<float>(fbxMatrix.Get(j, i));
		}
	}
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

}  // namespace FbxUtil

void Fbx::LoadModel(const std::string& path) {
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(path.c_str(), "", *mFbxManager, *mSettings);
	mFBXFilePath = path;
	
	if (!mSceneLoader->scene()) {
		std::cerr << "Error: Failed to load FBX scene from path: " << path << std::endl;
		return;
	}
	
	// Convert the scene to a standard coordinate system (Z-Up, Right-Handed) and units (meters)
	// to ensure consistency.
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mSceneLoader->scene());
	fbxsdk::FbxSystemUnit::m.ConvertScene(mSceneLoader->scene());
	fbxsdk::FbxNode* rootNode = mSceneLoader->scene()->GetRootNode();
	if (rootNode) {
		ProcessNode(rootNode);
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	
	std::string str = data.str();
	FbxUtil::MemoryStreamFbxStream fbxStream(str.data(), str.size());
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(&fbxStream, "", *mFbxManager, *mSettings);
	
	if (!mSceneLoader->scene()) {
		std::cerr << "Error: Failed to load FBX scene from stream" << std::endl;
		return;
	}
	
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mSceneLoader->scene());
	
	fbxsdk::FbxSystemUnit::m.ConvertScene(mSceneLoader->scene());
	
	fbxsdk::FbxNode* rootNode = mSceneLoader->scene()->GetRootNode();
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
	fbxsdk::FbxGeometryConverter geometryConverter(mFbxManager->manager());
	mesh = static_cast<fbxsdk::FbxMesh*>(geometryConverter.Triangulate(mesh, true));
	
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	auto& vertices = resultMesh->get_vertices();
	auto& indices = resultMesh->get_indices();
	
	int polygonCount = mesh->GetPolygonCount();
	fbxsdk::FbxVector4* controlPoints = mesh->GetControlPoints();
	
	for (int i = 0; i < polygonCount; ++i) {
		for (int j = 0; j < 3; ++j) { // Process each vertex of the triangle
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			auto vertex = std::make_unique<MeshVertex>();
			
			// Position
			fbxsdk::FbxVector4 position = controlPoints[controlPointIndex];
			vertex->set_position({static_cast<float>(position[0]), static_cast<float>(position[1]), static_cast<float>(position[2])});
			
			// Normal
			fbxsdk::FbxVector4 normal;
			if (mesh->GetPolygonVertexNormal(i, j, normal)) {
				vertex->set_normal({static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2])});
			}
			
			// Texture Coordinates (UVs)
			fbxsdk::FbxStringList uvSetNameList;
			mesh->GetUVSetNames(uvSetNameList);
			if (uvSetNameList.GetCount() > 0) {
				fbxsdk::FbxVector2 uv;
				bool unmapped;
				if (mesh->GetPolygonVertexUV(i, j, uvSetNameList.GetStringAt(0), uv, unmapped)) {
					vertex->set_texture_coords1({static_cast<float>(uv[0]), static_cast<float>(uv[1])});
				}
			}
			
			// Vertex Color
			if (mesh->GetElementVertexColorCount() > 0) {
				fbxsdk::FbxGeometryElementVertexColor* colorElement = mesh->GetElementVertexColor(0);
				fbxsdk::FbxColor color = colorElement->GetDirectArray().GetAt(colorElement->GetIndexArray().GetAt(mesh->GetPolygonVertexIndex(i) + j));
				vertex->set_color({static_cast<float>(color.mRed), static_cast<float>(color.mGreen), static_cast<float>(color.mBlue), static_cast<float>(color.mAlpha)});
			}
			
			vertices.push_back(std::move(vertex));
			indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
		}
	}
	
	// Process materials
	int materialCount = node->GetMaterialCount();
	std::vector<std::shared_ptr<MaterialProperties>> meshMaterials;
	meshMaterials.reserve(materialCount);
	for (int i = 0; i < materialCount; ++i) {
		fbxsdk::FbxSurfaceMaterial* material = node->GetMaterial(i);
		auto matPtr = std::make_shared<MaterialProperties>();
		
		// Extract properties
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
				if (!std::filesystem::is_regular_file(texturePath)) {
					texturePath = (std::filesystem::path(mFBXFilePath).parent_path() / std::filesystem::path(texturePath).filename()).string();
				}
				
				std::ifstream file(texturePath, std::ios::binary);
				if (file) {
					
					std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
					if (!buffer.empty()) {
						 matPtr->mTextureDiffuse = std::make_shared<nanogui::Texture>(buffer.data(), buffer.size());
						
						matPtr->mHasDiffuseTexture = true;
					}
				} else {
					std::cerr << "Warning: Could not open texture file: " << texturePath << std::endl;
				}
			}
		}
		meshMaterials.push_back(matPtr);
	}
	
	resultMesh->get_material_properties() = meshMaterials;
	mMaterialProperties.push_back(std::move(meshMaterials));
	
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
