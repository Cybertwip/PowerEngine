#include "Actor.hpp"

Actor::Actor(entt::registry& registry) : mRegistry(registry), mEntity(mRegistry.create()) {
	
}
