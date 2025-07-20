#pragma once

#include "actors/Actor.hpp"
#include "graphics/drawing/IMeshBatch.hpp"
#include "simulation/Primitive.hpp"

#include <memory>
#include <string>

class Actor;
class MeshData;
class ShaderWrapper;

class PrimitiveBuilder {
public:
	explicit PrimitiveBuilder(IMeshBatch& meshBatch);
	
	Actor& build(Actor& actor, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader);
	
	/**
	 * @brief Builds a sprite primitive actor.
	 * * Creates a two-sided flat mesh with the given dimensions and applies the specified texture.
	 * @param actor The actor to add the primitive components to.
	 * @param actorName The name of the actor.
	 * @param texturePath The file path for the diffuse texture.
	 * @param width The width of the sprite mesh.
	 * @param height The height of the sprite mesh.
	 * @param meshShader The shader to use for rendering.
	 * @return A reference to the configured actor.
	 */
	Actor& build_sprite(Actor& actor, const std::string& actorName, const std::string& texturePath, float width, float height, ShaderWrapper& meshShader);
	
private:
	IMeshBatch& mMeshBatch;
};
