#include "import/Fbx.hpp"

#include <algorithm>
#include <thread>
#include <filesystem>
#include <map>
#include <numeric>  // For std::iota
#include <glm/gtc/quaternion.hpp>

namespace FbxUtil {

class MemoryStreamFbxStream : public FbxStream {
public:
	MemoryStreamFbxStream(const char* data, size_t size)
	: mData(data), mSize(size), mPosition(0) {}
	
	virtual ~MemoryStreamFbxStream() override {}
	
	virtual EState GetState() override {
		return FbxStream::eOpen;
	}
	
	virtual bool Open(void* /*pStreamData*/) override {
		// Stream is already open
		mPosition = 0;
		return true;
	}
	
	virtual bool Close() override {
		// Nothing to close for memory stream
		return true;
	}
	
	virtual size_t Read(void* pBuffer, FbxUInt64 pSize) const override {
		size_t bytesToRead = static_cast<size_t>(std::min<FbxUInt64>(pSize, mSize - mPosition));
		memcpy(pBuffer, mData + mPosition, bytesToRead);
		mPosition += bytesToRead;
		return bytesToRead;
	}
	
	virtual size_t Write(const void* /*pBuffer*/, FbxUInt64 /*pSize*/) override {
		// Read-only stream
		return 0;
	}
	
	virtual int GetReaderID() const override {
		return 0;
	}
	
	virtual int GetWriterID() const override {
		return -1; // Not writable
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
			return; // Out of bounds
		}
		mPosition = static_cast<size_t>(newPos);
		return mPosition;
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
	
	virtual void ClearError() override {
		// No action needed
	}
	
private:
	const char* mData;
	size_t mSize;
	mutable size_t mPosition;
};

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

void Fbx::LoadModel(const std::string& path) {
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(path.c_str(), "", *mFbxManager, *mSettings);
	
	mFBXFilePath = path;
	
	if (mSceneLoader->scene()) {
		ProcessNode(mSceneLoader->scene()->GetRootNode());
	} else {
		std::cerr << "Error: Failed to load FBX scene from file " << path << std::endl;
	}
}

void Fbx::LoadModel(std::stringstream& data) {
	// Initialize the FBX manager and IO settings
	mFbxManager = std::make_unique<ozz::animation::offline::fbx::FbxManagerInstance>();
	mSettings = std::make_unique<ozz::animation::offline::fbx::FbxDefaultIOSettings>(*mFbxManager);
	
	// Extract data from the stringstream
	std::string str = data.str();
	const char* buffer = str.data();
	size_t size = str.size();
	
	// Create an instance of the custom FbxStream
	FbxUtil::MemoryStreamFbxStream fbxStream(buffer, size);
	
	// Load the scene using FbxSceneLoader with the custom stream
	mSceneLoader = std::make_unique<ozz::animation::offline::fbx::FbxSceneLoader>(&fbxStream, "", *mFbxManager, *mSettings);
	
	if (mSceneLoader->scene()) {
		// Process the root node of the scene
		ProcessNode(mSceneLoader->scene()->GetRootNode());
	} else {
		std::cerr << "Error: Failed to load FBX scene from stream" << std::endl;
	}
}

void Fbx::ProcessNode(fbxsdk::FbxNode* node) {
	if (!node) return;
	
	FbxMesh* mesh = node->GetMesh();
	if (mesh) {
		ProcessMesh(mesh, node);
	}
	
	// Process child nodes
	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i) {
		FbxNode* childNode = node->GetChild(i);
		ProcessNode(childNode);
	}
}

void Fbx::ProcessMesh(fbxsdk::FbxMesh* mesh, fbxsdk::FbxNode* node) {
	if (!mesh) {
		std::cerr << "Error: Invalid mesh pointer." << std::endl;
		return;
	}
	
	// Create a new MeshData instance and add it to mMeshes
	auto& resultMesh = mMeshes.emplace_back(std::make_unique<MeshData>());
	
	// Retrieve control points (vertices)
	int controlPointCount = mesh->GetControlPointsCount();
	FbxVector4* controlPoints = mesh->GetControlPoints();
	
	// Get global transform of the node
	FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
	
	// Reserve space for vertices and indices
	resultMesh->get_vertices().resize(controlPointCount);
	int polygonCount = mesh->GetPolygonCount();
	int indexCount = polygonCount * 3; // Assuming triangulated mesh
	resultMesh->get_indices().resize(indexCount);
	
	// Process each polygon
	for (int i = 0; i < polygonCount; ++i) {
		assert(mesh->GetPolygonSize(i) == 3 && "Mesh must be triangulated.");
		
		for (int j = 0; j < 3; ++j) {
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			
			// Get or create the vertex
			auto& vertexPtr = resultMesh->get_vertices()[controlPointIndex];
			if (vertexPtr.get() == nullptr) {
				vertexPtr = std::make_unique<MeshVertex>();
			}
			auto& vertex = *vertexPtr;
			
			// Get position
			FbxVector4 position = controlPoints[controlPointIndex];
			
			// Apply global transform
			FbxVector4 transformedPosition = globalTransform.MultT(position);
			
			// Convert to glm::vec3
			vertex.set_position({ static_cast<float>(transformedPosition[0]),
				static_cast<float>(transformedPosition[1]),
				static_cast<float>(transformedPosition[2]) });
			// Now, get normal
			// Get normal from mesh
			
			FbxVector4 normal;
			if (mesh->GetPolygonVertexNormal(i, j, normal)) {
				// Apply the global transform to the normal
				FbxVector4 transformedNormal = globalTransform.MultT(normal);
				
				// Convert to glm::vec3
				vertex.set_normal({ static_cast<float>(transformedNormal[0]),
					static_cast<float>(transformedNormal[1]),
					static_cast<float>(transformedNormal[2]) });
			} else {
				vertex.set_normal({ 0.0f, 1.0f, 0.0f }); // Default normal
			}
			
			// Get UVs
			FbxStringList uvSetNameList;
			mesh->GetUVSetNames(uvSetNameList);
			if (uvSetNameList.GetCount() > 0) {
				const char* uvSetName = uvSetNameList.GetStringAt(0);
				FbxVector2 uv;
				bool unmapped;
				if (mesh->GetPolygonVertexUV(i, j, uvSetName, uv, unmapped)) {
					vertex.set_texture_coords1({ static_cast<float>(uv[0]),
						static_cast<float>(uv[1]) });
				} else {
					vertex.set_texture_coords1(glm::vec2(0.0f, 0.0f)); // Default UV
				}
			} else {
				vertex.set_texture_coords1(glm::vec2(0.0f, 0.0f)); // Default UV
			}
			
			// Get color
			if (mesh->GetElementVertexColorCount() > 0) {
				// Get the color element
				FbxGeometryElementVertexColor* colorElement = mesh->GetElementVertexColor(0);
				
				// The mapping mode can be eByPolygonVertex or eByControlPoint
				FbxColor color;
				int colorIndex = 0;
				if (colorElement->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
					colorIndex = controlPointIndex;
				} else if (colorElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
					colorIndex = mesh->GetPolygonVertexIndex(i) + j;
				}
				if (colorElement->GetReferenceMode() == FbxGeometryElement::eDirect) {
					color = colorElement->GetDirectArray().GetAt(colorIndex);
				} else if (colorElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
					int id = colorElement->GetIndexArray().GetAt(colorIndex);
					color = colorElement->GetDirectArray().GetAt(id);
				}
				vertex.set_color({ static_cast<float>(color.mRed),
					static_cast<float>(color.mGreen),
					static_cast<float>(color.mBlue),
					static_cast<float>(color.mAlpha) });
			} else {
				vertex.set_color({ 1.0f, 1.0f, 1.0f, 1.0f }); // Default color
			}
			
			// Set material ID
			// This requires getting the material assigned to the polygon
			int materialIndex = 0;
			FbxLayerElementArrayTemplate<int>* materialIndices = nullptr;
			if (mesh->GetElementMaterial()) {
				materialIndices = &mesh->GetElementMaterial()->GetIndexArray();
				FbxGeometryElement::EMappingMode materialMappingMode = mesh->GetElementMaterial()->GetMappingMode();
				if (materialMappingMode == FbxGeometryElement::eByPolygon) {
					materialIndex = materialIndices->GetAt(i);
				} else if (materialMappingMode == FbxGeometryElement::eAllSame) {
					materialIndex = materialIndices->GetAt(0);
				}
			}
			vertex.set_material_id(materialIndex);
			
			// Assign index
			resultMesh->get_indices()[i * 3 + j] = controlPointIndex;
		}
	}
	
	// Process materials
	// Retrieve materials from the node
	int materialCount = node->GetMaterialCount();
	std::vector<std::shared_ptr<SerializableMaterialProperties>> serializableMaterials;
	serializableMaterials.reserve(materialCount);
	for (int i = 0; i < materialCount; ++i) {
		FbxSurfaceMaterial* material = node->GetMaterial(i);
		// Create SerializableMaterialProperties
		auto matPtr = std::make_shared<SerializableMaterialProperties>();
		SerializableMaterialProperties& matData = *matPtr;
		
		// Combine the path and index into a single string
		std::string combined = node->GetName() + std::to_string(i);
		
		// Compute the hash of the combined string
		std::size_t hashValue = std::hash<std::string>{}(combined);
		
		// Assign to mIdentifier
		matData.mIdentifier = hashValue;
		
		// Set material properties
		FbxPropertyT<FbxDouble3> ambientProperty = material->FindProperty(FbxSurfaceMaterial::sAmbient);
		FbxPropertyT<FbxDouble3> diffuseProperty = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		FbxPropertyT<FbxDouble3> specularProperty = material->FindProperty(FbxSurfaceMaterial::sSpecular);
		FbxPropertyT<FbxDouble> shininessProperty = material->FindProperty(FbxSurfaceMaterial::sShininess);
		FbxPropertyT<FbxDouble> opacityProperty = material->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);
		
		FbxDouble3 ambient = ambientProperty.IsValid() ? ambientProperty.Get() : FbxDouble3(0, 0, 0);
		FbxDouble3 diffuse = diffuseProperty.IsValid() ? diffuseProperty.Get() : FbxDouble3(0, 0, 0);
		FbxDouble3 specular = specularProperty.IsValid() ? specularProperty.Get() : FbxDouble3(0, 0, 0);
		double shininess = shininessProperty.IsValid() ? shininessProperty.Get() : 0.1;
		double opacity = opacityProperty.IsValid() ? 1.0 - opacityProperty.Get() : 1.0;
		
		matData.mAmbient = glm::vec4(ambient[0], ambient[1], ambient[2], 1.0f);
		matData.mDiffuse = glm::vec4(diffuse[0], diffuse[1], diffuse[2], 1.0f);
		matData.mSpecular = glm::vec4(specular[0], specular[1], specular[2], 1.0f);
		matData.mShininess = static_cast<float>(shininess);
		matData.mOpacity = static_cast<float>(opacity);
		matData.mHasDiffuseTexture = false;
		
		// Load texture if available
		FbxProperty diffusePropertyTexture = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		int textureCount = diffusePropertyTexture.GetSrcObjectCount<FbxFileTexture>();
		if (textureCount > 0) {
			FbxFileTexture* texture = diffusePropertyTexture.GetSrcObject<FbxFileTexture>(0);
			if (texture) {
				std::string textureFileName = texture->GetFileName();
				bool textureLoaded = false;
				
				// Texture is not embedded; try to load from file path
				// The texture file path can be absolute or relative
				// Try to resolve the file path
				std::string textureFilePath = textureFileName;
				
				// If the path is relative, make it relative to the FBX file
				if (!std::filesystem::path(textureFilePath).is_absolute()) {
					std::filesystem::path fbxFilePath = mFBXFilePath;
					std::filesystem::path fbxDirectory = fbxFilePath.parent_path();
					textureFilePath = (fbxDirectory / textureFilePath).string();
				}
				
				// Try to read the texture file
				std::ifstream textureFile(textureFilePath, std::ios::binary);
				if (textureFile) {
					// Read file into mTextureDiffuse
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
		
		// Add material to the list
		serializableMaterials.push_back(matPtr);
	}
	
	resultMesh->get_material_properties().resize(serializableMaterials.size());
	
	mMaterialProperties.push_back(std::move(serializableMaterials));
	
	// Ignore this for now
	 ProcessBones(mesh); // This call should be handled externally or after ProcessMesh
}
