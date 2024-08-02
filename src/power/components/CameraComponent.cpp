#include "components/CameraComponent.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"

CameraComponent::CameraComponent(TransformComponent& transformComponent, float fov, float near, float far, float aspect)
: mTransformComponent(transformComponent)
, mFov(fov)
, mNear(near)
, mFar(far)
, mAspect(aspect) {
	mProjection = nanogui::Matrix4f::perspective(mFov, mNear, mFar, mAspect);
}

void CameraComponent::update_view() {
	auto matrix = mTransformComponent.get_matrix();
	// Convert glm::mat4 to nanogui::Matrix4f
	std::memcpy(mView.m, glm::value_ptr(matrix), sizeof(float) * 16);
}
