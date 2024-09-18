#pragma once

#include "graphics/drawing/SkinnedMesh.hpp"

#include <string>

class Actor;
class MeshBatch;
class MeshComponent;
class ShaderWrapper;

class MeshActorBuilder {
public:
	MeshActorBuilder(SkinnedMesh::MeshBatch& meshBatch);

	Actor& build(Actor& actor, const std::string& path, ShaderWrapper& shader);
	
private:
	SkinnedMesh::MeshBatch& mMeshBatch;
};
