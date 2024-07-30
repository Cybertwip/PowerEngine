#include "ColorComponent.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

ColorComponent::ColorComponent(ShaderWrapper& shaderWrapper)
: mColor(1.0f, 1.0f, 1.0f)
, mShader(shaderWrapper) {
	
}

void ColorComponent::set_color(const glm::vec3& color) {
	mShader.set_uniform("color", glm_to_nanogui(color));
}
