#pragma once

#include "graphics/drawing/SkinnedMesh.hpp"

#include <string>

class Actor;
class MeshBatch;
class MeshComponent;


class MeshActorBuilder {
public:
	MeshActorBuilder(SkinnedMesh::SkinnedMeshShader& shader);
	
	Actor& build(Actor& actor, const std::string& path);
	
	
	SkinnedMesh::MeshBatch& mesh_batch() {
		return *mMeshBatch;
	}
	
private:
	SkinnedMesh::SkinnedMeshShader& mShader;
	
	std::unique_ptr<SkinnedMesh::MeshBatch> mMeshBatch;
};
