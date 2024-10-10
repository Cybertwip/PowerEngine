#pragma once

#include <glm/glm.hpp>
#include <optional>

class Actor;

class ICameraManager
{
public:
	ICameraManager() = default;
	virtual ~ICameraManager() = default;
	
	virtual void look_at(Actor& actor) = 0;
	
	virtual void look_at(const glm::vec3& position) = 0;
	
	virtual void set_transform(const glm::mat4& transform) = 0;
	
	virtual std::optional<glm::mat4> get_transform() = 0;
	
};
