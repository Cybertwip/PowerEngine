#pragma once

#include "MeshVertex.hpp"
#include "MaterialProperties.hpp"

#include <memory>
#include <vector>

class MeshData {
public:
	MeshData() = default;
		
	virtual ~MeshData() = default;
	
	std::vector<std::unique_ptr<MeshVertex>>& get_vertices() {
		return mVertices;
	}
	
	std::vector<unsigned int>& get_indices() {
		return mIndices;
	}
	
	std::vector<std::shared_ptr<MaterialProperties>>& get_material_properties() {
		return mMaterials;
	}
	
protected:
	std::vector<std::unique_ptr<MeshVertex>> mVertices;
	std::vector<unsigned int> mIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mMaterials;
};

class SkinnedMeshData : public MeshData {
public:
	SkinnedMeshData() = default;
	
	SkinnedMeshData(MeshData& meshData) : MeshData() {
		// This constructor needs to properly handle vertex conversion
		// from MeshVertex to SkinnedMeshVertex.
		auto vertexBackup = std::move(meshData.get_vertices());
		mIndices = meshData.get_indices();
		mMaterials = meshData.get_material_properties();
		
		mVertices.clear();
		
		for (auto& meshVertex : vertexBackup) {
			mVertices.push_back(std::make_unique<SkinnedMeshVertex>(*meshVertex));
		}
	}
	
	~SkinnedMeshData() override = default;
};
