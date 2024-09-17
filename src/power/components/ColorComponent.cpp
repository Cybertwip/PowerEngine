#include "ColorComponent.hpp"

#include "components/MetadataComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

ColorComponent::ColorComponent(MetadataComponent& metadataComponent, ShaderWrapper& shaderWrapper)
: mMetadataComponent(metadataComponent)
, mColor(1.0f, 1.0f, 1.0f, 1.0f)
, mShader(shaderWrapper) {
	
}

void ColorComponent::set_color(const glm::vec4& color) {
	mColor = color;
}

void ColorComponent::apply() {
	mShader.set_uniform("identifier", static_cast<int>(mMetadataComponent.identifier()));
	mShader.set_uniform("color", glm_to_nanogui(mColor));
}
