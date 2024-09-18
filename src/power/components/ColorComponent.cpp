#include "ColorComponent.hpp"

#include "components/MetadataComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

ColorComponent::ColorComponent(MetadataComponent& metadataComponent)
: mMetadataComponent(metadataComponent)
, mColor(1.0f, 1.0f, 1.0f, 1.0f)
, mVisible(true) {
	
}

void ColorComponent::set_color(const glm::vec4& color) {
	mColor = color;
}

void ColorComponent::set_visible(bool visible) {
	mVisible = visible;
}

void ColorComponent::apply_to(ShaderWrapper& shader) {
	shader.set_uniform("identifier", static_cast<int>(mMetadataComponent.identifier()));
	shader.set_uniform("color", glm_to_nanogui(mColor));
}
