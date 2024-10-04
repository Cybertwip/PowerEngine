// MeshDeserializer.cpp

#include "MeshDeserializer.hpp"

bool MeshDeserializer::load_mesh(CompressedSerialization::Deserializer& deserializer, const std::string& path) {
	// Clear any previously loaded meshes
	clear();
	
	bool allSuccess = true;
		
	if (path.empty()) {
		std::cerr << "MeshDeserializer: Asset has an empty precomputed path.\n";
		allSuccess = false;
		return;
	}
	
	
	std::filesystem::path filePath(path);
	
	std::string extension = filePath.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(),
				   [](unsigned char c) { return std::tolower(c); });
	
	// Determine the file type based on extension and deserialize accordingly
	bool success = false;
	if (extension == ".pma") { // Custom serialized mesh data format
		success = deserialize_pma(deserializer);
	}
	else if (extension == ".psk") { // Skeletal mesh format, handled without skeletons
		success = deserialize_psk(deserializer);
	}
	else {
		std::cerr << "MeshDeserializer: Unsupported file extension: " << extension << "\n";
		allSuccess = false;
		return;
	}
	
	if (!success) {
		std::cerr << "MeshDeserializer: Failed to deserialize meshes from: " << path << "\n";
		allSuccess = false;
	}
	
	if (allSuccess) {
		std::cout << "MeshDeserializer: Successfully deserialized all meshes.\n";
	} else {
		std::cerr << "MeshDeserializer: Some meshes failed to deserialize. Check errors above.\n";
	}
	
	return allSuccess;
}


bool MeshDeserializer::deserialize_pma(CompressedSerialization::Deserializer& deserializer) {
	// Deserialize mesh data count
	int32_t meshDataCount;
	if (!deserializer.read_int32(meshDataCount)) {
		std::cerr << "MeshDeserializer: Failed to read mesh data count in .pma file\n";
		return false;
	}
	
	m_meshes.reserve(meshDataCount);
	
	// Deserialize each MeshData
	for (int32_t i = 0; i < meshDataCount; ++i) {
		auto meshData = std::make_unique<MeshData>();
		if (!meshData->deserialize(deserializer)) {
			std::cerr << "MeshDeserializer: Failed to deserialize MeshData for mesh " << i << " in .pma file\n";
			return false;
		}
		m_meshes.push_back(std::move(meshData));
	}
	
	std::cout << "MeshDeserializer: Successfully deserialized " << meshDataCount << " mesh(es) from .pma file.\n";
	return true;
}

bool MeshDeserializer::deserialize_psk(CompressedSerialization::Deserializer& deserializer) {
	// .psk files typically contain skeletal mesh data. Since we are ignoring skeletons,
	// we'll focus solely on mesh data and skip any skeleton-related sections.
	
	// Deserialize mesh data count
	int32_t meshDataCount;
	if (!deserializer.read_int32(meshDataCount)) {
		std::cerr << "MeshDeserializer: Failed to read mesh data count in .psk file\n";
		return false;
	}
	
	m_meshes.reserve(meshDataCount);
	
	// Deserialize each MeshData
	for (int32_t i = 0; i < meshDataCount; ++i) {
		auto meshData = std::make_unique<SkinnedMeshData>();
		if (!meshData->deserialize(deserializer)) {
			std::cerr << "MeshDeserializer: Failed to deserialize MeshData for mesh " << i << " in .psk file\n";
			return false;
		}
		
		// Skip skeleton data if any (implementation depends on .psk format)
		// Assuming MeshData::deserialize handles only mesh data and ignores skeletons
		
		meshData->get_material_properties().clear();

		// Deserialize number of material properties (if applicable)
		int32_t materialCount;
		if (!deserializer.read_int32(materialCount)) {
			std::cerr << "Failed to read material count for mesh " << i << "\n";
			return false;
		}
		
		for (int32_t i = 0; i < materialCount; ++i) {
			SerializableMaterialProperties material;
			
			material.deserialize(deserializer);
			
			auto properties = std::make_shared<MaterialProperties>();
			
			properties->deserialize(material);
			
			meshData->get_material_properties().push_back(properties);
		}

		m_meshes.push_back(std::move(meshData));
	}
	
	m_skeleton = Skeleton();
	
	// Deserialize the skeleton
	if (!m_skeleton->deserialize(deserializer)) {
		
		m_skeleton = std::nullopt;
		
		std::cerr << "MeshDeserializer: Failed to deserialize skeleton in .psk file\n";
		return false;
	}
	

	
	std::cout << "MeshDeserializer: Successfully deserialized " << meshDataCount << " mesh(es) from .psk file.\n";
	return true;
}

std::vector<std::unique_ptr<MeshData>> MeshDeserializer::get_meshes() {
	return std::move(m_meshes);
}

std::optional<Skeleton> MeshDeserializer::get_skeleton() {
	return m_skeleton;
}


void MeshDeserializer::clear() {
	m_meshes.clear();
}
