#pragma once

#include "graphics/drawing/Mesh.hpp"

#include <string>
#include <unordered_map>

class Actor;
class MeshComponent;
class ShaderWrapper;
class SkinnedFbx;

struct BatchUnit;

class MeshActorBuilder {
public:
	MeshActorBuilder(BatchUnit& batches);

	Actor& build(Actor& actor, const std::string& path, ShaderWrapper& shader);
	
private:
	BatchUnit& mBatchUnit;
	std::unordered_map<std::string, std::unique_ptr<SkinnedFbx>> mModels;
};
