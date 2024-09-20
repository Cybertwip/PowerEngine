#pragma once

#include "graphics/drawing/Mesh.hpp"

#include <string>

class Actor;
class MeshComponent;
class ShaderWrapper;

struct BatchUnit;

class MeshActorBuilder {
public:
	MeshActorBuilder(BatchUnit& batches);

	Actor& build(Actor& actor, const std::string& path, ShaderWrapper& shader);
	
private:
	BatchUnit& mBatchUnit;
};
