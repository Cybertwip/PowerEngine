#ifndef ANIM_ENTITY_COMPONENT_LIGHT_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_LIGHT_COMPONENT_H

#include "../component.h"

#include "shading/light_manager.h"


#include <string>
#include <memory>
#include <vector>
#include <map>
#include <glm/glm.hpp>

namespace anim
{
    class DirectionalLightComponent : public ComponentBase<DirectionalLightComponent>
    {
    public:
        ~DirectionalLightComponent() = default;
        void update(std::shared_ptr<Entity> entity) override;
		std::shared_ptr<Component> clone() override {
			assert(false && "Not implemented");
			
			return nullptr;
		}
		
		void set_parameters(const LightManager::DirectionalLight& parameters);
		
		LightManager::DirectionalLight& get_parameters() {
			return _parameters;
		}

	private:
		LightManager::DirectionalLight _parameters;
		
		glm::vec3 _initial_direction;
		
		std::map<int, glm::mat4> _transformStack;

    };
}

#endif
