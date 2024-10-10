#pragma once

#include "simulation/PrimitiveShape.hpp"

class ICartridgeActorLoader {
public:
	ICartridgeActorLoader() = default;
	virtual ~ICartridgeActorLoader() = default;
	virtual Actor& create_actor(PrimitiveShape primitiveShape) = 0;
};
