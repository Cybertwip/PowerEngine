#pragma once


#include "animation/Skeleton.hpp"

#include "graphics/shading/MeshData.hpp"
#include "filesystem/MeshActorImporter.hpp"
#include "filesystem/CompressedSerialization.hpp"

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <optional>


/**
 * @class MeshDeserializer
 * @brief Responsible for deserializing mesh data from .pma and .psk files.
 *
 * This class provides functionality to load and deserialize mesh data
 * from `.pma` and `.psk` file formats without involving shaders,
 * batching systems, or actor components.
 */
class MeshDeserializer {
public:
	/**
	 * @brief Default constructor.
	 */
	MeshDeserializer() = default;
	
	/**
	 * @brief Loads and deserializes mesh data from the given CompressedMeshActor.
	 *
	 * @param actor The CompressedMeshActor containing compressed assets.
	 * @return True if all meshes are successfully deserialized, false otherwise.
	 */
	bool load_mesh(CompressedSerialization::Deserializer& deserializer, const std::string& path);

	/**
	 * @brief Retrieves the deserialized mesh data.
	 *
	 * @return A constant reference to a vector of unique pointers to MeshData.
	 */
	std::vector<std::unique_ptr<MeshData>> get_meshes();
	
	Skeleton* get_skeleton();

	/**
	 * @brief Clears all loaded mesh data.
	 */
	void clear();
	
private:
	/**
	 * @brief Internal storage for deserialized mesh data.
	 */
	std::vector<std::unique_ptr<MeshData>> m_meshes;
	
	/**
	 * @brief Helper function to deserialize mesh data from a .pma file.
	 *
	 * @param deserializer The deserializer instance.
	 * @return True if successful, false otherwise.
	 */
	bool deserialize_pma(CompressedSerialization::Deserializer& deserializer);
	
	/**
	 * @brief Helper function to deserialize mesh data from a .psk file.
	 *
	 * @param deserializer The deserializer instance.
	 * @return True if successful, false otherwise.
	 */
	bool deserialize_psk(CompressedSerialization::Deserializer& deserializer);
	
	std::unique_ptr<Skeleton> m_skeleton;
};

