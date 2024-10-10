#pragma once

#include "simulation/ICartridgeActorLoader.hpp"

class Actor;
class MeshActorLoader;
class ShaderWrapper;

class CartridgeActorLoader : public ICartridgeActorLoader {
public:
	CartridgeActorLoader(MeshActorLoader& meshActorLoader, ShaderWrapper& meshShader);
	Actor& create_actor(PrimitiveShape primitiveShape) override;
	
private:
	MeshActorLoader& mMeshActorLoader;
	ShaderWrapper& mMeshShader;
};
