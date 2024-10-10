#pragma once

#include <vector>

class Actor;
class IActorSelectedCallback;

class IActorVisualManager {
public:
	virtual ~IActorVisualManager() = default;

	virtual void add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) = 0;
	
	virtual void add_actor(std::reference_wrapper<Actor> actor) = 0;

	virtual void remove_actor(std::reference_wrapper<Actor> actor) = 0;
	virtual void remove_actors(std::vector<std::reference_wrapper<Actor>> actors) = 0;

	virtual void fire_actor_selected_event(std::optional<std::reference_wrapper<Actor>> actor) = 0;
};

class IActorSelectedRegistry {
public:
	virtual ~IActorSelectedRegistry() = default;
	virtual void RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) = 0;
	
	virtual void UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) = 0;
};
