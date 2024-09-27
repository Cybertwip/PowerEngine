#pragma once

#include "filesystem/CompressedSerialization.hpp"
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

	Actor& build(Actor& actor, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	Actor& build_mesh(Actor& actor, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& build_skinned(Actor& actor, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

private:
	BatchUnit& mBatchUnit;
};
