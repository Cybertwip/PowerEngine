#pragma once

#include "simulation/ICartridgeActorLoader.hpp"

class Actor;
class IActorVisualManager;
class MeshActorLoader;
class ShaderWrapper;

class CartridgeActorLoader : public ICartridgeActorLoader {
public:
	CartridgeActorLoader(MeshActorLoader& meshActorLoader, IActorVisualManager& actorVisualManager, ShaderWrapper& meshShader);
	Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape) override;
	
private:
	MeshActorLoader& mMeshActorLoader;
	IActorVisualManager& mActorVisualManager;
	ShaderWrapper& mMeshShader;
};
