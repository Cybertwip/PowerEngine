#include "simulation/CartridgeActorLoader.hpp"

#include "MeshActorLoader.hpp"

CartridgeActorLoader::CartridgeActorLoader(MeshActorLoader& meshActorLoader, ShaderWrapper& meshShader) :
mMeshActorLoader(meshActorLoader)
, mMeshShader(meshShader)
{
	
}

Actor& CartridgeActorLoader::create_actor(PrimitiveShape primitiveShape) {
	return mMeshActorLoader.create_actor(primitiveShape, mMeshShader);
}





