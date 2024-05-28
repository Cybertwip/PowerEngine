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
glm::mat4 TransformComponent::get_mat4() const
{
	glm::mat4 rotation = glm::toMat4(glm::quat(mRotation));
	
	return glm::translate(glm::mat4(1.0f), mTranslation) * rotation * glm::scale(glm::mat4(1.0f), mScale);
}
glm::vec3 &TransformComponent::get_translation()
{
	return mTranslation;
}
glm::vec3 &TransformComponent::get_rotation()
{
	return mRotation;
}
glm::vec3 &TransformComponent::get_scale()
{
	return mScale;
}
TransformComponent &TransformComponent::set_translation(const glm::vec3 &vec)
{
	mTranslation = vec;
	return *this;
}
TransformComponent &TransformComponent::set_scale(const glm::vec3 &vec)
{
	mScale = vec;
	
	return *this;
}
TransformComponent &TransformComponent::set_scale(float scale)
{
	set_scale(glm::vec3{scale, scale, scale});
	return *this;
}

TransformComponent &TransformComponent::set_rotation(const glm::vec3 &vec)
{
	mRotation = vec;
	
	return *this;
}
TransformComponent &TransformComponent::set_transform(const glm::mat4 &mat)
{
	auto [t, r, s] = DecomposeTransform(mat);
	mTranslation = t;
	mRotation = glm::eulerAngles(r);
	mScale = s;
	return *this;
}

}
