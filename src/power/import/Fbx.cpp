#include "Fbx.hpp"

#include <algorithm>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>

namespace FbxUtil {


// Helper function to convert FbxMatrix to glm::mat4
glm::mat4 ToGlmMat4(const fbxsdk::FbxAMatrix& fbxMatrix) {
	glm::mat4 glmMat;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			glmMat[i][j] = static_cast<float>(fbxMatrix.Get(j, i));
		}
	}
	return glmMat;
}

class MemoryStreamFbxStream : public FbxStream {
public:
	MemoryStreamFbxStream(const char* data, size_t size)
	: mData(data), mSize(size), mPosition(0) {}
	
	virtual ~MemoryStreamFbxStream() override {}
	
	virtual EState GetState() override {
		return FbxStream::eOpen;
	}
	
	virtual bool Open(void* /*pStreamData*/) override {
		mPosition = 0;
		return true;
	}
	
	virtual bool Close() override {
		return true;
	}
	
	virtual size_t Read(void* pBuffer, FbxUInt64 pSize) const override {
		size_t bytesToRead = static_cast<size_t>(std::min<FbxUInt64>(pSize, mSize - mPosition));
		memcpy(pBuffer, mData + mPosition, bytesToRead);
		mPosition += bytesToRead;
		return bytesToRead;
	}
	
	virtual size_t Write(const void* /*pBuffer*/, FbxUInt64 /*pSize*/) override {
		return 0;
	}
	
	virtual int GetReaderID() const override {
		return 0;
	}
	
	virtual int GetWriterID() const override {
		return -1;
	}
	
	virtual bool Flush() override {
		return true;
	}
	
	virtual void Seek(const int64_t& pOffset, const FbxFile::ESeekPos& pSeekPos) override {
		int64_t newPos = 0;
		switch (pSeekPos) {
			case FbxFile::eBegin:
				newPos = pOffset;
				break;
			case FbxFile::eCurrent:
				newPos = static_cast<int64_t>(mPosition) + pOffset;
				break;
			case FbxFile::eEnd:
				newPos = static_cast<int64_t>(mSize) + pOffset;
				break;
			default:
				return;
		}
		if (newPos < 0 || newPos > static_cast<int64_t>(mSize)) {
			return;
		}
		mPosition = static_cast<size_t>(newPos);
	}
	
	virtual int64_t GetPosition() const override {
		return static_cast<int64_t>(mPosition);
	}
	
	virtual void SetPosition(FbxInt64 pPosition) override {
		if (pPosition >= 0 && pPosition <= static_cast<FbxInt64>(mSize)) {
			mPosition = static_cast<size_t>(pPosition);
		}
	}
	
	virtual int GetError() const override {
		return 0;
	}
	
	virtual void ClearError() override {}
	
private:
	const char* mData;
	size_t mSize;
	mutable size_t mPosition;
};


glm::mat4 GetUpAxisRotation(int up_axis, int up_axis_sign) {
	glm::mat4 rotation = glm::mat4(1.0f);
	if (up_axis == 0) {
		if (up_axis_sign == 1) {
			rotation = glm::rotate(rotation, glm::radians(90.0f), glm::vec3(0, 0, 1));
		} else {
			rotation = glm::rotate(rotation, glm::radians(-90.0f), glm::vec3(0, 0, 1));
		}
	} else if (up_axis == 1) {
		if (up_axis_sign == -1) {
			rotation = glm::rotate(rotation, glm::radians(180.0f), glm::vec3(1, 0, 0));
		}
	} else if (up_axis == 2) {
		if (up_axis_sign == 1) {
			rotation = glm::rotate(rotation, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		} else {
			rotation = glm::rotate(rotation, glm::radians(90.0f), glm::vec3(1, 0, 0));
		}
	}
	return rotation;
}

}  // namespace FbxUtil

void Fbx::LoadModel(const std::string& path) {
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(path.c_str(), "", *mFbxManager, *mSettings);
	mFBXFilePath = path;
	
	if (!mSceneLoader->scene()) {
		std::cerr << "Error: Failed to load FBX scene from stream" << std::endl;
		return;
	}
	
	// *** FIX 1: Coordinate System Conversion ***
	// FBX scenes can have different coordinate systems (Y-up, Z-up, etc.).
	// We convert the entire scene to a standard system (Y-Up, Right-Handed, meters).
	// This ensures consistency when rendering.
	// Convert the scene to a Z-Up, Right-Handed system.
	fbxsdk::FbxAxisSystem ourAxisSystem(fbxsdk::FbxAxisSystem::eZAxis, fbxsdk::FbxAxisSystem::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
	ourAxisSystem.ConvertScene(mSceneLoader->scene());
	
	fbxsdk::FbxSystemUnit::m.ConvertScene(mSceneLoader->scene());
	
	fbxsdk::FbxNode* rootNode = mSceneLoader->scene()->GetRootNode();
	if (rootNode) {
		// Start processing from the root node with an identity matrix.
		ProcessNode(rootNode);
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	
	std::string str = data.str();
	const char* buffer = str.data();
	size_t size = str.size();
	
	FbxUtil::MemoryStreamFbxStream fbxStream(buffer, size);
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(&fbxStream, "", *mFbxManager, *mSettings);
	
	if (!mSceneLoader->scene()) {
		std::cerr << "Error: Failed to load FBX scene from stream" << std::endl;
		return;
	}
	
	// *** Apply the same coordinate system fix here ***
	// Convert the scene to a Z-Up, Right-Handed system.
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
		
	// Process the mesh on the current node, if it exists
	fbxsdk::FbxMesh* mesh = node->GetMesh();
	if (mesh) {
		ProcessMesh(mesh, node);
	}
	
	// Recursively process all child nodes
	for (int i = 0; i < node->GetChildCount(); ++i) {
		ProcessNode(node->GetChild(i));
	}
}

void Fbx::ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	FbxGeometryConverter geometryConverter(mFbxManager->manager());
	
	// Critical: Triangulate always
	mesh = static_cast<fbxsdk::FbxMesh*>(geometryConverter.Triangulate(mesh, false));

	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	std::vector<std::unique_ptr<MeshVertex>>& vertices = resultMesh->get_vertices();
	std::vector<uint32_t>& indices = resultMesh->get_indices();
	
	int polygonCount = mesh->GetPolygonCount();
	for (int i = 0; i < polygonCount; ++i) {
		int polygonSize = mesh->GetPolygonSize(i);
		std::vector<uint32_t> polygonIndices;
		for (int j = 0; j < polygonSize; ++j) {
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			auto vertex = std::make_unique<MeshVertex>();
			fbxsdk::FbxVector4 position = mesh->GetControlPointAt(controlPointIndex);
			vertex->set_position({ static_cast<float>(position[0]),
				static_cast<float>(position[1]),
				static_cast<float>(position[2]) });
			
			fbxsdk::FbxVector4 normal;
			if (mesh->GetPolygonVertexNormal(i, j, normal)) {
				vertex->set_normal({ static_cast<float>(normal[0]),
					static_cast<float>(normal[1]),
					static_cast<float>(normal[2]) });
			} else {
				vertex->set_normal({ 0.0f, 1.0f, 0.0f });
			}
			
			fbxsdk::FbxStringList uvSetNameList;
			mesh->GetUVSetNames(uvSetNameList);
			if (uvSetNameList.GetCount() > 0) {
				const char* uvSetName = uvSetNameList.GetStringAt(0);
				fbxsdk::FbxVector2 uv;
				bool unmapped;
				if (mesh->GetPolygonVertexUV(i, j, uvSetName, uv, unmapped)) {
					vertex->set_texture_coords1({ static_cast<float>(uv[0]),
						static_cast<float>(uv[1]) });
				} else {
					vertex->set_texture_coords1(glm::vec2(0.0f, 0.0f));
				}
			} else {
				vertex->set_texture_coords1(glm::vec2(0.0f, 0.0f));
			}
			
			if (mesh->GetElementVertexColorCount() > 0) {
				fbxsdk::FbxGeometryElementVertexColor* colorElement = mesh->GetElementVertexColor(0);
				fbxsdk::FbxColor color;
				int colorIndex = 0;
				if (colorElement->GetMappingMode() == fbxsdk::FbxGeometryElement::eByControlPoint) {
					colorIndex = controlPointIndex;
				} else if (colorElement->GetMappingMode() == fbxsdk::FbxGeometryElement::eByPolygonVertex) {
					colorIndex = mesh->GetPolygonVertexIndex(i) + j;
				}
				if (colorElement->GetReferenceMode() == fbxsdk::FbxGeometryElement::eDirect) {
					color = colorElement->GetDirectArray().GetAt(colorIndex);
				} else if (colorElement->GetReferenceMode() == fbxsdk::FbxGeometryElement::eIndexToDirect) {
					int id = colorElement->GetIndexArray().GetAt(colorIndex);
					color = colorElement->GetDirectArray().GetAt(id);
				}
				vertex->set_color({ static_cast<float>(color.mRed),
					static_cast<float>(color.mGreen),
					static_cast<float>(color.mBlue),
					static_cast<float>(color.mAlpha) });
			} else {
				vertex->set_color({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
			
			int materialIndex = 0;
			fbxsdk::FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;
			if (mesh->GetElementMaterial()) {
				materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
				fbxsdk::FbxGeometryElement::EMappingMode materialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
				if (materialMappingMode == fbxsdk::FbxGeometryElement::eByPolygon) {
					materialIndex = materialIndices->GetAt(i);
				} else if (materialMappingMode == fbxsdk::FbxGeometryElement::eAllSame) {
					materialIndex = materialIndices->GetAt(0);
				}
			}
			vertex->set_material_id(materialIndex);
			
			vertices.push_back(std::move(vertex));
			polygonIndices.push_back(static_cast<uint32_t>(vertices.size() - 1));
		}
		
		for (size_t j = 1; j < polygonIndices.size() - 1; ++j) {
			indices.push_back(polygonIndices[0]);
			indices.push_back(polygonIndices[j]);
			indices.push_back(polygonIndices[j + 1]);
		}
	}
	
	int materialCount = node->GetMaterialCount();
	std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
	serializableMaterials.reserve(materialCount);
	for (int i = 0; i < materialCount; ++i) {
		fbxsdk::FbxSurfaceMaterial* material = node->GetMaterial(i);
		auto matPtr = std::make_shared<SerializableMaterialProperties>();
		SerializableMaterialProperties& matData = *matPtr;
		
		std::string combined = node->GetName() + std::to_string(i);
		std::size_t hashValue = std::hash<std::string>{}(combined);
		matData.mIdentifier = hashValue;
		
		fbxsdk::FbxPropertyT<fbxsdk::FbxDouble3> ambientProperty = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sAmbient);
		fbxsdk::FbxPropertyT<fbxsdk::FbxDouble3> diffuseProperty = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sDiffuse);
		fbxsdk::FbxPropertyT<fbxsdk::FbxDouble3> specularProperty = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sSpecular);
		fbxsdk::FbxPropertyT<fbxsdk::FbxDouble> shininessProperty = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sShininess);
		fbxsdk::FbxPropertyT<fbxsdk::FbxDouble> opacityProperty = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sTransparencyFactor);
		
		fbxsdk::FbxDouble3 ambient = ambientProperty.IsValid() ? ambientProperty.Get() : fbxsdk::FbxDouble3(0, 0, 0);
		fbxsdk::FbxDouble3 diffuse = diffuseProperty.IsValid() ? diffuseProperty.Get() : fbxsdk::FbxDouble3(0, 0, 0);
		fbxsdk::FbxDouble3 specular = specularProperty.IsValid() ? specularProperty.Get() : fbxsdk::FbxDouble3(0, 0, 0);
		double shininess = shininessProperty.IsValid() ? shininessProperty.Get() : 0.1;
		double opacity = opacityProperty.IsValid() ? 1.0 - opacityProperty.Get() : 1.0;
		
		matData.mAmbient = glm::vec4(static_cast<float>(ambient[0]), static_cast<float>(ambient[1]), static_cast<float>(ambient[2]), 1.0f);
		matData.mDiffuse = glm::vec4(static_cast<float>(diffuse[0]), static_cast<float>(diffuse[1]), static_cast<float>(diffuse[2]), 1.0f);
		matData.mSpecular = glm::vec4(static_cast<float>(specular[0]), static_cast<float>(specular[1]), static_cast<float>(specular[2]), 1.0f);
		matData.mShininess = static_cast<float>(shininess);
		matData.mOpacity = static_cast<float>(opacity);
		matData.mHasDiffuseTexture = false;
		
		fbxsdk::FbxProperty diffusePropertyTexture = material->FindProperty(fbxsdk::FbxSurfaceMaterial::sDiffuse);
		int textureCount = diffusePropertyTexture.GetSrcObjectCount<fbxsdk::FbxFileTexture>();
		if (textureCount > 0) {
			fbxsdk::FbxFileTexture* texture = diffusePropertyTexture.GetSrcObject<fbxsdk::FbxFileTexture>(0);
			if (texture) {
				std::string textureFileName = texture->GetFileName();
				bool textureLoaded = false;
				
				std::string textureFilePath = textureFileName;
				if (!std::filesystem::path(textureFilePath).is_absolute()) {
					std::filesystem::path fbxFilePath = mFBXFilePath;
					std::filesystem::path fbxDirectory = fbxFilePath.parent_path();
					textureFilePath = (fbxDirectory / textureFilePath).string();
				}
				
				std::ifstream textureFile(textureFilePath, std::ios::binary);
				if (textureFile) {
					matData.mTextureDiffuse.assign(std::istreambuf_iterator<char>(textureFile),
												   std::istreambuf_iterator<char>());
					matData.mHasDiffuseTexture = true;
					textureLoaded = true;
				} else {
					std::cerr << "Warning: Unable to open texture file: " << textureFilePath << std::endl;
				}
				
				if (!textureLoaded) {
					std::cerr << "Warning: Texture data could not be loaded for material: " << material->GetName() << std::endl;
				}
			}
		}
		
		serializableMaterials.push_back(matPtr);
	}
	
	resultMesh->get_material_properties().resize(serializableMaterials.size());
	mMaterialProperties.push_back(std::move(serializableMaterials));
	
	ProcessBones(mesh);
}

std::vector<std::unique_ptr<MeshData>>& Fbx::GetMeshData() {
	return mMeshes;
}

void Fbx::SetMeshData(std::vector<std::unique_ptr<MeshData>>&& meshData) {
	mMeshes = std::move(meshData);
}

std::vector<std::vector<std::shared_ptr<SerializableMaterialProperties>>>& Fbx::GetMaterialProperties() {
	return mMaterialProperties;
}

bool Fbx::SaveTo(const std::string& filename) const {
	return false;
}

bool Fbx::LoadFrom(const std::string& filename) {
	return false;
}

void Fbx::ProcessBones(fbxsdk::FbxMesh* mesh) {
}
