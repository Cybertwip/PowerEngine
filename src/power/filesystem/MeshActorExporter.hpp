#pragma once

#include "filesystem/CompressedSerialization.hpp"

// Include FBX SDK headers
#include <fbxsdk.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <ostream>

// Forward declarations of custom classes
class MeshData;
class MeshDeserializer;
class SerializableMaterialProperties;
class SkinnedFbx;

class MeshActorExporter {
public:
	// Constructor and Destructor
	MeshActorExporter();
	~MeshActorExporter();
	
	// Synchronous export to file
	bool exportToFile(CompressedSerialization::Deserializer& deserializer,
					  const std::string& sourcePath,
					  const std::string& exportPath);
	
	// Synchronous export to stream
	bool exportToStream(CompressedSerialization::Deserializer& deserializer,
						const std::string& sourcePath,
						std::ostream& outStream);
	
	// Asynchronous export to file
	void exportToFileAsync(CompressedSerialization::Deserializer& deserializer,
						   const std::string& sourcePath,
						   const std::string& exportPath,
						   std::function<void(bool success)> callback);
	
	// Asynchronous export to stream
	void exportToStreamAsync(std::unique_ptr<CompressedSerialization::Deserializer> deserializerPtr,
							 const std::string& sourcePath,
							 std::ostream& outStream,
							 std::function<void(bool success)> callback);
	
	bool exportSkeleton(Skeleton& skeleton, const std::string& exportPath);
	
	bool exportSkeleton(Skeleton& skeleton, std::ostream& outStream);
	
	void exportSkinnedFbxToStream(SkinnedFbx& skinnedFbx, std::ostream& outStream);

private:
	
	bool setupScene(CompressedSerialization::Deserializer& deserializer,
									   const std::string& sourcePath,
									   FbxManager* fbxManager,
					FbxScene* scene);
	// Helper functions
	void createMaterials(FbxManager* fbxManager,
						 FbxScene* scene,
						 const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials,
						 std::map<int, FbxSurfacePhong*>& materialMap);
	
	void createMesh(MeshData& meshData,
					const std::map<int, FbxSurfacePhong*>& materialMap,
					FbxMesh* fbxMesh,
					FbxNode* meshNode);
	
	// Member variables
	std::unique_ptr<MeshDeserializer> mMeshDeserializer;
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::vector<FbxNode*> mMeshModels;
};
