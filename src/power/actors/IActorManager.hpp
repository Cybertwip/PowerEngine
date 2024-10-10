#pragma once

class Actor;

class IActorManager {
public:
	IActorManager() = default;
	virtual ~IActorManager() = default;
	
	virtual Actor& create_actor() = 0;
	virtual void remove_actor(Actor& actor) = 0;
	virtual void remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) = 0;
};

