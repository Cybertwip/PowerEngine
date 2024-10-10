#pragma once

#include "components/TransformComponent.hpp"

#include <glm/glm.hpp>
#include <functional>
#include <optional>

class Actor;

class ICameraManager
{
public:
	ICameraManager() = default;
	virtual ~ICameraManager() = default;
	
	virtual void look_at(Actor& actor) = 0;
	
	virtual void look_at(const glm::vec3& position) = 0;
		
	virtual std::optional<std::reference_wrapper<TransformComponent>> get_transform_component() = 0;

};
