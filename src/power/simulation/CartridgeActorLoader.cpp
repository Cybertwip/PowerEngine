#include "simulation/CartridgeActorLoader.hpp"

#include "actors/IActorSelectedRegistry.hpp"
#include "actors/ActorManager.hpp"

#include "MeshActorLoader.hpp"

CartridgeActorLoader::CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, AnimationTimeProvider& animationTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) :
mMeshActorLoader(meshActorLoader)
, mActorManager(actorManager)
, mActorVisualManager(actorVisualManager)
, mAnimationTimeProvider(animationTimeProvider)
, mMeshShader(meshShader)
, mSkinnedMeshShader(skinnedShader)
{
	
}

void CartridgeActorLoader::cleanup() {
	mActorVisualManager.remove_actors(mLoadedActors);
	mLoadedActors.clear();
}

Actor& CartridgeActorLoader::create_actor(const std::string& filePath) {
	Actor& actor = mMeshActorLoader.create_actor(filePath, mAnimationTimeProvider, mMeshShader, mSkinnedMeshShader);
	
	mActorVisualManager.add_actor(actor);
	
	mLoadedActors.push_back(actor);
	
	return actor;
}

Actor& CartridgeActorLoader::create_actor(const std::string& actorName, PrimitiveShape primitiveShape) {
	Actor& actor = mMeshActorLoader.create_actor(actorName, primitiveShape, mMeshShader);
	
	mActorVisualManager.add_actor(actor);
	
	mLoadedActors.push_back(actor);
	
	return actor;
}





