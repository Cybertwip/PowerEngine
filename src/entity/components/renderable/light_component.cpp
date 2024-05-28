#include "light_component.h"
#include "../../../graphics/shader.h"
#include "../../../graphics/mesh.h"
#include "../../entity.h"

#include "utility.h"

namespace anim
{
void DirectionalLightComponent::update(std::shared_ptr<Entity> entity)
{
	auto t = entity->get_component<TransformComponent>()->get_translation();
	auto r = entity->get_component<TransformComponent>()->get_rotation();

	_parameters.position = t;
	_parameters.direction = r;
}


void DirectionalLightComponent::set_parameters(const LightManager::DirectionalLight& parameters){
	_parameters = parameters;
}
}

