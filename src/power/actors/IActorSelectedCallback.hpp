#pragma once

#include <optional>
#include <functional>

// Forward declaration of Actor class
class Actor;

// Interface for actor selection callbacks
class IActorSelectedCallback {
public:
	virtual ~IActorSelectedCallback() = default;
	virtual void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) = 0;
};

