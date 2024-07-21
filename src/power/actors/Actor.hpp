#pragma once

#include <entt/entt.hpp>

class Actor {
public:
    Actor(entt::registry& registry);
    
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

private:
    entt::registry& mRegistry;
    entt::entity mEntity;
};

