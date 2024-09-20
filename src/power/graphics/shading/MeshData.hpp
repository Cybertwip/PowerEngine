#pragma once

#include "MeshVertex.hpp"
#include "MaterialProperties.hpp"

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
	
private:
	std::vector<MeshVertex> mVertices;
	std::vector<unsigned int> mIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mMaterials;
};

struct SkinnedMeshData {
	SkinnedMeshData(MeshData& meshData) : mMeshData(meshData) {
		auto& vertices = meshData.get_vertices();
		
		for (auto& meshVertex : vertices) {
			mVertices.push_back(SkinnedMeshVertex(meshVertex));
		}
	}
	
	std::vector<SkinnedMeshVertex>& get_skinned_vertices() {
		return mVertices;
	}
	
	std::vector<unsigned int>& get_indices() {
		return mMeshData.get_indices();
	}
	
	std::vector<std::shared_ptr<MaterialProperties>>& get_material_properties() const {
		return mMeshData.get_material_properties();
	}
	
private:
	MeshData& mMeshData;
	std::vector<SkinnedMeshVertex> mVertices;
};

