#pragma once

#include "simulation/ICartridgeActorLoader.hpp"

class Actor;
class IActorVisualManager;
class MeshActorLoader;
class ShaderWrapper;

class CartridgeActorLoader : public ICartridgeActorLoader {
public:
	CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, ShaderWrapper& meshShader);
	Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape) override;
	
	void cleanup();
	
private:
	MeshActorLoader& mMeshActorLoader;
	IActorManager& mActorManager;
	IActorVisualManager& mActorVisualManager;
	ShaderWrapper& mMeshShader;
	
	std::vector<std::reference_wrapper<Actor>> mLoadedActors;
};
