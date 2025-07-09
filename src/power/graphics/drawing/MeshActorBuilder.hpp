#pragma once

#include <memory>
#include <sstream>
#include <string>

// Forward declarations to reduce header dependencies
class Actor;
class AnimationTimeProvider;
class ShaderWrapper;
class ModelImporter; // Changed from MeshActorImporter
struct BatchUnit;

/**
 * @class MeshActorBuilder
 * @brief Constructs Actor objects from model files or data streams.
 * This class handles the loading of mesh, skeleton, and animation data using
 * the ModelImporter and assembles the necessary components for a renderable Actor.
 */
class MeshActorBuilder {
public:
    /**
     * @brief Constructor for MeshActorBuilder.
     * @param batchUnit A reference to the BatchUnit which manages rendering batches for meshes.
     */
    explicit MeshActorBuilder(BatchUnit& batchUnit);

    /**
     * @brief Builds an actor from a model file specified by a path.
     * @param actor The actor to build upon.
     * @param timeProvider The main time provider for animations.
     * @param previewTimeProvider The time provider for previewing animations.
     * @param path The filesystem path to the model file.
     * @param meshShader The shader to use for non-skinned meshes.
     * @param skinnedShader The shader to use for skinned meshes.
     * @return A reference to the configured actor.
     */
    Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

    /**
     * @brief Builds an actor from a model file provided as a memory stream.
     * @param actor The actor to build upon.
     * @param timeProvider The main time provider for animations.
     * @param previewTimeProvider The time provider for previewing animations.
     * @param dataStream A stringstream containing the model file data.
     * @param path A string representing the original path or name, used for identification and determining file type.
     * @param meshShader The shader to use for non-skinned meshes.
     * @param skinnedShader The shader to use for skinned meshes.
     * @return A reference to the configured actor.
     */
    Actor& build(Actor& actor, AnimationTimeProvider& timeProvider, AnimationTimeProvider& previewTimeProvider, std::stringstream& dataStream, const std::string& path, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);

private:
    /**
     * @brief Private helper that constructs the actor from the processed model data.
     * This is the core logic that adds components based on the data loaded by the ModelImporter.
     */
    Actor& build_from_model_data(
        Actor& actor,
        AnimationTimeProvider& timeProvider,
        AnimationTimeProvider& previewTimeProvider,
        std::unique_ptr<ModelImporter> importer,
        const std::string& path,
        const std::string& actorName,
        ShaderWrapper& meshShader,
        ShaderWrapper& skinnedShader
    );

    BatchUnit& mBatchUnit;
    // The mMeshActorImporter member is no longer needed and has been removed.
};
