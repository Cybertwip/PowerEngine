#pragma once

class IActorSelectedCallback;

class IActorSelectedRegistry {
public:
	virtual ~IActorSelectedRegistry() = default;
	virtual void RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) = 0;
	
	virtual void UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) = 0;
};
