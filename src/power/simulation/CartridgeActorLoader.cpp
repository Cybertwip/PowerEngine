#include "simulation/CartridgeActorLoader.hpp"

#include "MeshActorLoader.hpp"

CartridgeActorLoader::CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorVisualManager actorVisualManager, ShaderWrapper& meshShader) :
mMeshActorLoader(meshActorLoader)
, mActorVisualManager(actorVisualManager)
, mMeshShader(meshShader)
{
	
}

Actor& CartridgeActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape) {
	Actor& actor = mMeshActorLoader.create_actor(actorName, primitiveShape, mMeshShader);
	
	mActorVisualManager.add_actor(actor)
	
	return actor;
}





