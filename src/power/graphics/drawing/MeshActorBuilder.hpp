#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include "graphics/drawing/Mesh.hpp"

#include <memory>
#include <sstream>
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

	Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::stringstream& fbxStream, const std::string& actorName, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& build_mesh(Actor& actor, AnimationTimeProvider& timeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

	Actor& build_skinned(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& actorName, CompressedSerialization::Deserializer& deserializer, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

private:
	BatchUnit& mBatchUnit;
	
	
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
};
