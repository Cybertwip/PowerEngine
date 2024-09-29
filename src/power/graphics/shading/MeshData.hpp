#pragma once

#include "MeshVertex.hpp"
#include "MaterialProperties.hpp"

#include "filesystem/CompressedSerialization.hpp"

#include <memory>
#include <vector>

struct MeshData {
	MeshData() {
		
	}
	
	MeshData(const MeshData& other)
	: mVertices(other.mVertices), // Copy vertices
	mIndices(other.mIndices),   // Copy indices
	mMaterials(other.mMaterials) // Shallow copy of shared pointers
	{
		// If you need a deep copy of mMaterials, you could iterate over the vector and copy each element explicitly.
	}

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
	virtual void serialize(CompressedSerialization::Serializer& serializer) const {
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
	virtual bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		
		mVertices.clear();
		
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
	
protected:
	std::vector<MeshVertex> mVertices;
	std::vector<unsigned int> mIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mMaterials;
};

struct SkinnedMeshData : public MeshData {
	SkinnedMeshData() = default;
	
	SkinnedMeshData(MeshData& meshData) : MeshData(meshData) {
		
		auto vertexBackup = mVertices;
		
		mVertices.clear();
		
		for (auto& meshVertex : vertexBackup) {
			mVertices.push_back(SkinnedMeshVertex(meshVertex));
		}
	}
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const override {
		// Serialize SkinnedMeshVertex vector
		uint32_t skinnedVertexCount = static_cast<uint32_t>(mVertices.size());
		serializer.write_uint32(skinnedVertexCount);
		for (const auto& vertex : mVertices) {
			
			const auto& skinnedVertex = static_cast<const SkinnedMeshVertex&>(vertex);

			skinnedVertex.serialize(serializer);
		}
		
		// Serialize indices
		uint32_t indexCount = static_cast<uint32_t>(mIndices.size());
		serializer.write_uint32(indexCount);
		for (const auto& index : mIndices) {
			serializer.write_uint32(index);
		}
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) override {
		// Deserialize SkinnedMeshVertex vector
		
		mVertices.clear();
		
		uint32_t skinnedVertexCount = 0;
		if (!deserializer.read_uint32(skinnedVertexCount)) return false;
		mVertices.resize(skinnedVertexCount); // Initialize with a default MeshVertex
		
		for (int i = 0; i < mVertices.size(); ++i) {
			SkinnedMeshVertex deserializable;
			if (!deserializable.deserialize(deserializer)) return false;
			
			mVertices[i] = deserializable;
		}
		
		mIndices.clear();
		
		// Deserialize indices
		uint32_t indexCount = 0;
		if (!deserializer.read_uint32(indexCount)) return false;
		mIndices.resize(indexCount);
		for (auto& index : mIndices) {
			if (!deserializer.read_uint32(index)) return false;
		}

		return true;
	}
};

