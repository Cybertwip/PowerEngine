#include "MeshActorLoader.hpp"

#include "actors/ActorManager.hpp"

#include "animation/Animation.hpp"
#include "animation/Skeleton.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "import/ModelImporter.hpp"

#include "simulation/PrimitiveBuilder.hpp"

// Use the provided ImageUtils to get texture dimensions.
#include "filesystem/ImageUtils.hpp"

MeshActorLoader::MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit, MeshActorBuilder& actorBuilder)
: mActorManager(actorManager)
, mPrimitiveBuilder(std::make_unique<PrimitiveBuilder>(batchUnit.mMeshBatch))
, mBatchUnit(batchUnit)
, mMeshActorBuilder(actorBuilder){
	
}

MeshActorLoader::~MeshActorLoader() {
}

const BatchUnit& MeshActorLoader::get_batch_unit() {
	return mBatchUnit;
}

Actor& MeshActorLoader::create_actor(const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	Actor& actor = mMeshActorBuilder.build(mActorManager.create_actor(), timeProvider, path, meshShader, skinnedShader);
	
	auto& transformComponent = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);
	
	return actor;
	
}

Actor& MeshActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	return mPrimitiveBuilder->build(mActorManager.create_actor(), actorName, primitiveShape, meshShader);
}

Actor& MeshActorLoader::create_sprite_actor(const std::string& actorName, const std::string& texturePath, ShaderWrapper& meshShader) {
	// 1. Get texture dimensions to create a mesh of the same size.
	int width = 0;
	int height = 0;
	int channels = 0;
	std::vector<uint8_t> pixels; // Required by the function, but we only need the dimensions here.
	
	// Use the ImageUtils function to read the PNG and get its dimensions.
	if (!read_png_file_to_vector(texturePath, pixels, width, height, channels)) {
		// Error handling: texture not found or is invalid.
		// For now, we'll default to a 100x100 sprite and log an error.
		width = 100;
		height = 100;
		// In a real application, you would use a proper logging system.
		fprintf(stderr, "Warning: Could not load texture dimensions for '%s'. Defaulting to 100x100.\n", texturePath.c_str());
	}
	
	// 2. Create an actor and build the sprite primitive with the correct dimensions and texture.
	return mPrimitiveBuilder->build_sprite(
										   mActorManager.create_actor(),
										   actorName,
										   texturePath,
										   static_cast<float>(width),
										   static_cast<float>(height),
										   meshShader
										   );
}


std::unique_ptr<Actor> MeshActorLoader::create_unique_actor(entt::registry& registry, const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) {
	
	auto unique_actor = std::make_unique<Actor>(registry);
	
	auto& actor = mMeshActorBuilder.build(*unique_actor, timeProvider, path, meshShader, skinnedShader);
	
	auto& transformComponent = actor.add_component<TransformComponent>();
	actor.add_component<TransformAnimationComponent>(transformComponent, timeProvider);
	
	return std::move(unique_actor);
}



std::unique_ptr<Actor> MeshActorLoader::create_unique_actor(entt::registry& registry, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader) {
	
	auto unique_actor = std::make_unique<Actor>(registry);
	
	auto& _ = mPrimitiveBuilder->build(*unique_actor, actorName, primitiveShape, meshShader);
	
	return std::move(unique_actor);
}
