#pragma once

#include "simulation/Primitive.hpp"

#include <memory>
#include <string>

class IMeshBatch;
class Actor;
class MeshData;
class ShaderWrapper;

class PrimitiveBuilder {
public:
	/**
	 * @brief Constructs a PrimitiveBuilder with access to MeshBatch.
	 *
	 * @param meshBatch Reference to the mesh batching system.
	 */
	explicit PrimitiveBuilder(IMeshBatch& meshBatch);
	
	/**
	 * @brief Builds a PrimitiveComponent based on the specified PrimitiveShape and adds it to the Actor.
	 *
	 * @param actor The Actor to which the PrimitiveComponent will be added.
	 * @param primitiveShape The type of primitive to create (e.g., Cube, Sphere, Cuboid).
	 * @param meshShader The ShaderWrapper to be used for the Mesh.
	 * @return Reference to the modified Actor.
	 */
	Actor& build(Actor& actor, const std::string& actorName, PrimitiveShape primitiveShape, ShaderWrapper& meshShader);
	
private:
	/**
	 * @brief Creates MeshData for the specified PrimitiveShape.
	 *
	 * @param primitiveShape The type of primitive to create.
	 * @return A unique pointer to the created MeshData.
	 */
	std::unique_ptr<MeshData> create_mesh_data(PrimitiveShape primitiveShape);
	
	IMeshBatch& mMeshBatch;
};
