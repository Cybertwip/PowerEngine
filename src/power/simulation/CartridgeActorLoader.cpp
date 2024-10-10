#include "simulation/CartridgeActorLoader.hpp"

#include "actors/IActorSelectedRegistry.hpp"
#include "actors/ActorManager.hpp"

#include "MeshActorLoader.hpp"

CartridgeActorLoader::CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, ShaderWrapper& meshShader) :
mMeshActorLoader(meshActorLoader)
, mActorManager(actorManager)
, mActorVisualManager(actorVisualManager)
, mMeshShader(meshShader)
{
	
}

void CartridgeActorLoader::cleanup() {
	mActorVisualManager.remove_actors(mLoadedActors);
	mLoadedActors.clear();
}

Actor& CartridgeActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape) {
	Actor& actor = mMeshActorLoader.create_actor(actorName, primitiveShape, mMeshShader);
	
	mActorVisualManager.add_actor(actor);
	
	mLoadedActors.push_back(actor);
	
	return actor;
}





