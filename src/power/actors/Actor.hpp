#pragma once

#include <entt/entt.hpp>

#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <memory>

class CancellationToken {
public:
	CancellationToken() : cancelled(false) {}
	
	void cancel() {
		cancelled.store(true);
		for (auto& callback : callbacks) {
			if (callback) {
				callback();
			}
		}
	}
	
	bool is_cancelled() const {
		return cancelled.load();
	}
	
	void register_callback(std::function<void()> callback) {
		callbacks.push_back(callback);
	}
	
private:
	std::atomic<bool> cancelled;
	std::vector<std::function<void()>> callbacks;
};

class Actor {
public:
    Actor(entt::registry& registry);

	virtual ~Actor() {
		deregister();
	}
	
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
	
	void register_on_deallocated(std::function<void()> onDeallocated) {
		mCancellationToken->register_callback(onDeallocated);
	}

private:
	void deregister() {
		mCancellationToken->cancel();
		mRegistry.destroy(mEntity);
	}
	
    entt::registry& mRegistry;
    entt::entity mEntity;
	std::unique_ptr<CancellationToken> mCancellationToken;
};

