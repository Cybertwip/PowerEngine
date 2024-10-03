#pragma once

#include "filesystem/CompressedSerialization.hpp"
#include "filesystem/MeshActorImporter.hpp"

#include "graphics/drawing/Mesh.hpp"

#include <memory>
#include <string>
#include <unordered_map>


class Actor;
class AnimationTimeProvider;
class MeshActorImporter;
class MeshComponent;
class ShaderWrapper;
class SkinnedFbx;

struct BatchUnit;

class MeshActorBuilder {
public:
	MeshActorBuilder(BatchUnit& batches);

	Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	Actor& build_mesh(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& build_skinned(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

private:
	BatchUnit& mBatchUnit;
	
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
};
