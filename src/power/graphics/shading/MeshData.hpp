#pragma once

#include "MeshVertex.hpp"
#include "MaterialProperties.hpp"

#include "filesystem/CompressedSerialization.hpp"

#include <memory>
#include <vector>

struct MeshData {
	std::vector<MeshVertex>& get_vertices() {
		return mVertices;
	}
	
	std::vector<unsigned int>& get_indices() {
		return mIndices;
	}
	
	std::vector<std::shared_ptr<MaterialProperties>>& get_material_properties() {
		return mMaterials;
	}
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		// Serialize vertices
		uint32_t vertexCount = static_cast<uint32_t>(mVertices.size());
		serializer.write_uint32(vertexCount);
		for (const auto& vertex : mVertices) {
			vertex.serialize(serializer);
		}
		
		// Serialize indices
		uint32_t indexCount = static_cast<uint32_t>(mIndices.size());
		serializer.write_uint32(indexCount);
		for (const auto& index : mIndices) {
			serializer.write_uint32(index);
		}
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		// Deserialize vertices
		uint32_t vertexCount = 0;
		if (!deserializer.read_uint32(vertexCount)) return false;
		mVertices.resize(vertexCount);
		for (auto& vertex : mVertices) {
			if (!vertex.deserialize(deserializer)) return false;
		}
		
		// Deserialize indices
		uint32_t indexCount = 0;
		if (!deserializer.read_uint32(indexCount)) return false;
		mIndices.resize(indexCount);
		for (auto& index : mIndices) {
			if (!deserializer.read_uint32(index)) return false;
		}
		return true;
	}
	
private:
	std::vector<MeshVertex> mVertices;
	std::vector<unsigned int> mIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mMaterials;
};

struct SkinnedMeshData {
	SkinnedMeshData() = default;
	
	SkinnedMeshData(std::unique_ptr<MeshData> meshData) : mMeshData(std::move(meshData)) {
		auto& vertices = mMeshData->get_vertices();
		
		for (auto& meshVertex : vertices) {
			mSkinnedVertices.push_back(SkinnedMeshVertex(meshVertex));
		}
	}
	
	std::vector<SkinnedMeshVertex>& get_skinned_vertices() {
		return mSkinnedVertices;
	}
	
	std::vector<unsigned int>& get_indices() {
		return mMeshData->get_indices();
	}
	
	std::vector<std::shared_ptr<MaterialProperties>>& get_material_properties() const {
		return mMeshData->get_material_properties();
	}
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		// Serialize the underlying MeshData
		mMeshData->serialize(serializer);
		
		// Serialize SkinnedMeshVertex vector
		uint32_t skinnedVertexCount = static_cast<uint32_t>(mSkinnedVertices.size());
		serializer.write_uint32(skinnedVertexCount);
		for (const auto& skinnedVertex : mSkinnedVertices) {
			skinnedVertex.serialize(serializer);
		}
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		// Deserialize the underlying MeshData
		
		mMeshData = std::make_unique<MeshData>();
		
		if (!mMeshData->deserialize(deserializer)) return false;
		
		// Deserialize SkinnedMeshVertex vector
		uint32_t skinnedVertexCount = 0;
		if (!deserializer.read_uint32(skinnedVertexCount)) return false;
		mSkinnedVertices.resize(skinnedVertexCount, SkinnedMeshVertex(mMeshData->get_vertices()[0])); // Initialize with a default MeshVertex
		
		for (auto& skinnedVertex : mSkinnedVertices) {
			if (!skinnedVertex.deserialize(deserializer)) return false;
		}
		
		return true;
	}

private:
	std::unique_ptr<MeshData> mMeshData;
	std::vector<SkinnedMeshVertex> mSkinnedVertices;
};

