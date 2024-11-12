#pragma once

#include "simulation/ICartridgeActorLoader.hpp"

class Actor;
class AnimationTimeProvider;
class IActorVisualManager;
class MeshActorLoader;
class Primitive;
class ShaderWrapper;

class CartridgeActorLoader : public ICartridgeActorLoader {
public:
	CartridgeActorLoader(VirtualMachine& virtualMachine, MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, AnimationTimeProvider& animationTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedMeshShader);
	
	Primitive* create_actor(PrimitiveShape primitiveShape) override;
	
	Actor& create_actor(const std::string& filePath) override;
	
	void cleanup();
	
private:
	VirtualMachine& mVirtualMachine;
	MeshActorLoader& mMeshActorLoader;
	IActorManager& mActorManager;
	IActorVisualManager& mActorVisualManager;
	AnimationTimeProvider& mAnimationTimeProvider;
	ShaderWrapper& mMeshShader;
	ShaderWrapper& mSkinnedMeshShader;

	std::vector<std::reference_wrapper<Actor>> mLoadedActors;
	std::vector<std::unique_ptr<Primitive>> mLoadedPrimitives;
};
