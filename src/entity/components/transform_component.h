#ifndef ANIM_ENTITY_COMPONENT_TRANSFORM_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_TRANSFORM_COMPONENT_H

#include "component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace anim
{
    class TransformComponent : public ComponentBase<TransformComponent>
    {
    public:
		TransformComponent(){
			update_transform();
		}
		const glm::mat4 &get_mat4();
		const glm::vec3 &get_translation();
		const glm::vec3 &get_rotation();
		const glm::vec3 &get_scale();
        TransformComponent &set_translation(glm::vec3 &vec);
        TransformComponent &set_scale(glm::vec3 &vec);
        TransformComponent &set_scale(float scale);
        TransformComponent &set_rotation(glm::vec3 &vec);
        TransformComponent &set_transform(const glm::mat4 &mat);
		std::shared_ptr<Component> clone() override {
			auto clone = std::make_shared<TransformComponent>();
			
			clone->mTranslation = mTranslation;
			clone->mScale = mScale;
			clone->mRotation = mRotation;
			clone->mTransform = mTransform;
			
			return clone;
		}
		
	private:
		void update_transform();
		
		glm::vec3 mTranslation{0.0f, 0.0f, 0.0f};
		glm::vec3 mScale{1.f, 1.f, 1.f};
		glm::vec3 mRotation{0.0f, 0.0f, 0.0f};

		glm::mat4 mTransform;

    };
}
#endif
