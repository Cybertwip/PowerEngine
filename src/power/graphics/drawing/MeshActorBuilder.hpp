#pragma once

#include "filesystem/MeshActorImporter.hpp" // For FbxData struct

#include <memory>
#include <sstream>
#include <string>

// Forward declarations
class Actor;
class AnimationTimeProvider;
class ShaderWrapper;
struct BatchUnit;

class MeshActorBuilder {
public:
	MeshActorBuilder(BatchUnit& batches);
	
	/**
	 * @brief Builds an actor from an FBX file specified by a path.
	 */
	Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	/**
	 * @brief Builds an actor from an FBX file provided as a memory stream.
	 * @param path A string representing the original path or name, used for identification.
	 */
	Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::stringstream& fbxStream, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
private:
	/**
	 * @brief Private helper that constructs the actor from the processed FbxData.
	 */
	Actor& build_from_fbx_data(
							   Actor& actor,
							   AnimationTimeProvider& timeProvider,
							   AnimationTimeProvider& previewTimeProvider,
							   std::unique_ptr<MeshActorImporter::FbxData> fbxData,
							   const std::string& path,
							   const std::string& actorName,
							   ShaderWrapper& meshShader,
							   ShaderWrapper& skinnedShader
							   );
	
	BatchUnit& mBatchUnit;
	std::unique_ptr<MeshActorImporter> mMeshActorImporter;
};
