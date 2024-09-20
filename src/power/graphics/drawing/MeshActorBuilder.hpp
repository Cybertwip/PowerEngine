#pragma once

#include "graphics/drawing/Mesh.hpp"

#include <string>

class Actor;
class MeshBatch;
class MeshComponent;
class ShaderWrapper;

class MeshActorBuilder {
public:
	MeshActorBuilder(std::vector<std::reference_wrapper<Batch>>& batches);

	Actor& build(Actor& actor, const std::string& path, ShaderWrapper& shader);
	
private:
	std::vector<std::reference_wrapper<Batch>>& mMeshBatches;
};
