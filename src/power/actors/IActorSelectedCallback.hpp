#pragma once

// Forward declaration of Actor class
class Actor;

// Interface for actor selection callbacks
class IActorSelectedCallback {
public:
	virtual ~IActorSelectedCallback() = default;
	virtual void OnActorSelected(Actor& actor) = 0;
};

