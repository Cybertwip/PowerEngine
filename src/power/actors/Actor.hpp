#pragma once

#include "serialization/UUID.hpp" // Include our new UUID header
#include <entt/entt.hpp>

class Actor {
public:
    // Constructor for creating a brand new Actor
    Actor(entt::registry& registry) : mRegistry(registry), mEntity(mRegistry.create()) {
        // Automatically add a new, unique IDComponent upon creation.
        add_component<IDComponent>(UUIDGenerator::generate());
    }

    // Constructor for wrapping an existing entity (used during deserialization)
    Actor(entt::registry& registry, entt::entity entity) : mRegistry(registry), mEntity(entity) {}

    virtual ~Actor() {
        // The registry owner (ActorManager) should handle destruction
        // to ensure all relationships are maintained. But if an Actor is
        // destroyed directly, we still clean up its entity.
        if (mRegistry.valid(mEntity)) {
            mRegistry.destroy(mEntity);
        }
    }

    // --- Component methods remain the same ---
    template<typename T>
    bool find_component() {
        return mRegistry.any_of<T>(mEntity);
    }

    template<typename T>
    T& get_component() {
        return mRegistry.get<T>(mEntity);
    }

    template<typename Type, typename... Args>
    Type& add_component(Args &&...args) {
        return mRegistry.emplace<Type>(mEntity, std::forward<Args>(args)...);
    }

    template<typename Type>
    void remove_component() {
        mRegistry.erase<Type>(mEntity);
    }
    
    entt::entity get_entity() const {
        return mEntity;
    }

	UUID identifier() {
        return get_component<IDComponent>().uuid;
	}


private:
    entt::registry& mRegistry;
    entt::entity mEntity;
};
