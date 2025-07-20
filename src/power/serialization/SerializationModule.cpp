#include "serialization/SerializationModule.hpp"
#include "serialization/SceneSerializer.hpp"
#include "serialization/BlueprintSerializer.hpp" // ADDED

#include "actors/ActorManager.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"

// ADDED: Include new/relevant components
#include "components/BlueprintMetadataComponent.hpp"
#include "components/BlueprintComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "execution/NodeProcessor.hpp"


SerializationModule::SerializationModule(ActorManager& actorManager, MeshActorBuilder& actorBuilder, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader)
: mActorManager(actorManager)
, mMeshActorBuilder(actorBuilder)
, mTimeProvider(timeProvider)
, mMeshShader(meshShader)
, mSkinnedMeshShader(skinnedShader)
, mSceneSerializer(std::make_unique<SceneSerializer>()) {
    // Register all serializable components
    mSceneSerializer->register_component<IDComponent>();
    mSceneSerializer->register_component<TransformComponent>();
    mSceneSerializer->register_component<CameraComponent>();
	mSceneSerializer->register_component<MetadataComponent>();
	mSceneSerializer->register_component<ModelMetadataComponent>();
	mSceneSerializer->register_component<BlueprintMetadataComponent>(); // MODIFIED
}

void SerializationModule::save_scene(const std::string& filepath) {
    // Note: The caller is now responsible for saving each blueprint to a file
    // (e.g., via blueprintComponent.save_blueprint()) and ensuring its entity
    // has a BlueprintMetadataComponent before calling this function.
    mSceneSerializer->serialize(mActorManager.registry(), filepath);
}

void SerializationModule::load_scene(const std::string& filepath) {
    mActorManager.clear_actors();

    // Deserialize the scene file, which populates entities with metadata components
    mSceneSerializer->deserialize(mActorManager.registry(), filepath);
    
    // Create one serializer to reuse for all blueprints
    BlueprintSerializer blueprint_serializer;
    
    // Iterate through all newly created entities to perform post-processing
    auto view = mActorManager.registry().view<IDComponent>();
    for (auto entity_handle : view) {
        auto& actor = mActorManager.create_actor(entity_handle);
        
        // Post-process models, as before
        if (actor.find_component<ModelMetadataComponent>()) {
            auto& modelMetadata = actor.get_component<ModelMetadataComponent>();
            mMeshActorBuilder.build(actor, mTimeProvider, modelMetadata.model_path(), mMeshShader, mSkinnedMeshShader);
        }
        
        // ADDED: Post-process blueprints
        if (actor.find_component<BlueprintMetadataComponent>()) {
            auto& blueprintMetadata = actor.get_component<BlueprintMetadataComponent>();
            
            // 1. Add a new, empty BlueprintComponent to the actor
            auto& blueprint_comp = actor.add_component<BlueprintComponent>(std::make_unique<NodeProcessor>());

            // 2. Use the BlueprintSerializer to deserialize the graph from its file
            //    into the component's NodeProcessor.
            blueprint_serializer.deserialize(blueprint_comp.node_processor(), blueprintMetadata.blueprint_path());
        }
    }
}
