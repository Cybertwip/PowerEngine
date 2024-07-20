#pragma once

#include <entt/entt.hpp>

class Actor {
public:
    Actor(entt::registry& registry);
    
    template<typename T>
    T& get_component() {
        return mRegistry.get<T>(mEntity);
    }

    template<typename T>
    T& add_component() {
        return mRegistry.emplace<T>(mEntity);
    }

private:
    entt::registry& mRegistry;
    entt::entity mEntity;
};

