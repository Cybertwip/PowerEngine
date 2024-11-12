#pragma once

#include "simulation/Primitive.hpp"

#include <string>

class Actor;
class Primitive;

class ICartridgeActorLoader {
public:
	ICartridgeActorLoader() = default;
	virtual ~ICartridgeActorLoader() = default;
	virtual Primitive* create_actor(PrimitiveShape primitiveShape) = 0;
	virtual Actor& create_actor(const std::string& filePath) = 0;
};
