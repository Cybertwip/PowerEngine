#include "Actor.hpp"

Actor::Actor(entt::registry& registry) : mRegistry(registry), mEntity(mRegistry.create()) {
	
}

Actor::~Actor() {
	if (mRegistry.valid(mEntity)) {
		mRegistry.destroy(mEntity);
	}
}
