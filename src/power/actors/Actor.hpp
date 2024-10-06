#pragma once

#include <entt/entt.hpp>

class Actor {
public:
    Actor(entt::registry& registry);

	virtual ~Actor();
	
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
	
	long long identifier() {
		static_assert(sizeof(this) <= sizeof(long long), "Pointer size is too large for long long type.");
		return reinterpret_cast<long long>(this);
	}

private:
    entt::registry& mRegistry;
    entt::entity mEntity;
};

