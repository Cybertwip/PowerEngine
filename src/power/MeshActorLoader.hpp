#pragma once

#include "simulation/Primitive.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>

class Actor;
class ActorManager;
class AnimationTimeProvider;
class BatchUnit;
class MeshActorBuilder;
class PrimitiveBuilder;
class ShaderManager;
class ShaderWrapper;

struct BatchUnit;

class MeshActorLoader
{
public:
	explicit MeshActorLoader(ActorManager& actorManager, ShaderManager& shaderManager, BatchUnit& batchUnit, MeshActorBuilder& meshActorBuilder);
	
	~MeshActorLoader();
	
	Actor& create_actor(const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader);
	
	/**
	 * @brief Creates an actor with a two-sided, textured sprite mesh.
	 * * The sprite's dimensions will match the dimensions of the provided texture.
	 * @param actorName The name for the new actor.
	 * @param texturePath The file path to the texture for the sprite.
	 * @param meshShader The shader to use for rendering the sprite.
	 * @return A reference to the newly created actor.
	 */
	Actor& create_sprite_actor(const std::string& actorName, const std::string& texturePath, ShaderWrapper& meshShader);
	
	std::unique_ptr<Actor> create_unique_actor(entt::registry& registry, const std::string& path, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	std::unique_ptr<Actor> create_unique_actor(entt::registry& registry, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader);
	
	const BatchUnit& get_batch_unit();
	
private:
	ActorManager& mActorManager;
	
	MeshActorBuilder& mMeshActorBuilder;
	std::unique_ptr<PrimitiveBuilder> mPrimitiveBuilder;
	
	BatchUnit& mBatchUnit;
};
