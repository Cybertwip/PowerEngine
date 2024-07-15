#include "transform_component.h"
#include "../../util/utility.h"

#include "entity/entity.h"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


namespace anim
{

const glm::mat4& TransformComponent::get_mat4()
{
	update_transform();
	return mTransform;
}

const glm::vec3 &TransformComponent::get_translation()
{
	return mTranslation;
}

const glm::vec3 &TransformComponent::get_rotation()
{
	return mRotation;
}

const glm::vec3 &TransformComponent::get_scale()
{
	return mScale;
}

TransformComponent &TransformComponent::set_translation(glm::vec3 &vec)
{
	mTranslation = vec;
	update_transform();
	return *this;
}

TransformComponent &TransformComponent::set_scale(glm::vec3 &vec)
{
	mScale = vec;
	update_transform();
	return *this;
}

TransformComponent &TransformComponent::set_scale(float scale)
{
	auto scaleRef = glm::vec3{scale, scale, scale};
	return set_scale(scaleRef);
}

TransformComponent &TransformComponent::set_rotation(glm::vec3 &vec)
{
	mRotation = vec;
	update_transform();
	return *this;
}

TransformComponent &TransformComponent::set_transform(const glm::mat4 &mat)
{
	auto [t, r, s] = DecomposeTransform(mat);
	mTranslation = t;
	mRotation = glm::eulerAngles(r);
	mScale = s;
	mTransform = mat;
	return *this;
}

// Helper method to update the transformation matrix
void TransformComponent::update_transform()
{
	glm::mat4 rotation = glm::toMat4(glm::quat(mRotation));
	
	mTransform = glm::translate(glm::mat4(1.0f), mTranslation) * rotation * glm::scale(glm::mat4(1.0f), mScale);
}

}
