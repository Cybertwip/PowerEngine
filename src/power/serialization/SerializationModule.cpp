#include "serialization/SerializationModule.hpp"
#include "serialization/SceneSerializer.hpp"

#include "actors/ActorManager.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"


SerializationModule::SerializationModule(ActorManager& actorManager, MeshActorBuilder& actorBuilder, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader)
: mActorManager(actorManager)
, mMeshActorBuilder(actorBuilder)
, mTimeProvider(timeProvider)
, mMeshShader(meshShader)
, mSkinnedMeshShader(skinnedShader) {
	// IMPORTANT: Register all serializable components
	
    mSceneSerializer.register_component<IDComponent>();
	mSceneSerializer.register_component<TransformComponent>();
	mSceneSerializer.register_component<CameraComponent>();
	mSceneSerializer.register_component<ModelMetadataComponent>();

}

void SerializationModule::save_scene(const std::string& filepath) {	
	mSceneSerializer.serialize(mActorManager.registry(), filepath);
}

void SerializationModule::load_scene(const std::string& filepath) {
	
    mActorManager.clear_actors();

	mSceneSerializer.deserialize(mActorManager.registry(), filepath);
	
	// After deserializing, the registry is full of entities with components.
	// We now need to re-create our C++ Actor wrappers to manage them.
	auto view = mActorManager.registry().view<IDComponent>();
	for (auto entity_handle : view) {
		// Use the Actor constructor that takes an existing entity handle
		auto& actor = mActorManager.create_actor(entity_handle);
		
		if (actor.find_component<ModelMetadataComponent>()) {
			auto& modelMetadataComponent = actor.get_component<ModelMetadataComponent>();
			
			mMeshActorBuilder.build(actor, mTimeProvider, modelMetadataComponent.model_path(), mMeshShader, mSkinnedMeshShader);
		}
	}
}

