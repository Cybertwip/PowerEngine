#pragma once

#include "simulation/ICartridgeActorLoader.hpp"

class Actor;
class AnimationTimeProvider;
class IActorVisualManager;
class MeshActorLoader;
class ShaderWrapper;

class CartridgeActorLoader : public ICartridgeActorLoader {
public:
	CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, AnimationTimeProvider& animationTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedMeshShader);
	Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape) override;
	
	Actor& create_actor(const std::string& filePath) override;
	
	void cleanup();
	
private:
	MeshActorLoader& mMeshActorLoader;
	IActorManager& mActorManager;
	IActorVisualManager& mActorVisualManager;
	AnimationTimeProvider& mAnimationTimeProvider;
	ShaderWrapper& mMeshShader;
	ShaderWrapper& mSkinnedMeshShader;

	std::vector<std::reference_wrapper<Actor>> mLoadedActors;
};
