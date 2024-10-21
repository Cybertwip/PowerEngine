#pragma once

#include "simulation/PrimitiveShape.hpp"

#include <string>

class Actor;

class ICartridgeActorLoader {
public:
	ICartridgeActorLoader() = default;
	virtual ~ICartridgeActorLoader() = default;
	virtual Actor& create_actor(const std::string& actorName, PrimitiveShape primitiveShape) = 0;
};
