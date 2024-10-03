#pragma once

// Include your internal structures
#include "filesystem/MeshActorImporter.hpp" // Adjust the path as needed
#include "graphics/shading/MeshData.hpp"
#include "animation/Skeleton.hpp"
#include "animation/Animation.hpp"

#include <SmallFBX.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cassert>
#include <functional> // For std::function

class MeshDeserializer;

class MeshActorExporter {
public:
	MeshActorExporter();
	~MeshActorExporter();
	
	// Existing synchronous methods
	bool exportToFile(CompressedSerialization::Deserializer& actor, const std::string& sourcePath, const std::string& exportPath);
	
	bool exportToStream(CompressedSerialization::Deserializer& deserializer,
						const std::string& sourcePath,
						std::ostream& outStream);
	
	// New asynchronous methods
	/**
	 * @brief Asynchronously exports to a file.
	 *
	 * @param actor The deserializer object.
	 * @param sourcePath The source path.
	 * @param exportPath The destination export path.
	 * @param callback A callback function that takes a bool indicating success or failure.
	 */
	void exportToFileAsync(CompressedSerialization::Deserializer& actor,
						   const std::string& sourcePath,
						   const std::string& exportPath,
						   std::function<void(bool success)> callback);
	
	/**
	 * @brief Asynchronously exports to a stream.
	 *
	 * @param deserializer The deserializer object.
	 * @param sourcePath The source path.
	 * @param outStream The output stream to write to.
	 * @param callback A callback function that takes a bool indicating success or failure.
	 */
	void exportToStreamAsync(std::unique_ptr<CompressedSerialization::Deserializer> deserializerPtr,
							 const std::string& sourcePath,
							 std::ostream& outStream,
							 std::function<void(bool success)> callback);
	
	private:
	// Internal Data
	std::vector<std::unique_ptr<MeshData>> mMeshes;
	std::vector<std::shared_ptr<sfbx::Mesh>> mMeshModels;
	std::unique_ptr<Skeleton> mSkeleton;
	std::unique_ptr<MeshDeserializer> mMeshDeserializer;
	
	// Helper Functions
	void createMaterials(std::shared_ptr<sfbx::Document> document,
						 const std::vector<std::shared_ptr<SerializableMaterialProperties>>& materials,
						 std::map<int, std::shared_ptr<sfbx::Material>>& materialMap);
	
	void createMesh(MeshData& meshData,
					const std::map<int, std::shared_ptr<sfbx::Material>>& materialMap,
					std::shared_ptr<sfbx::Mesh> parentModel);
};
