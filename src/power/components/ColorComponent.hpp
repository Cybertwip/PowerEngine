#pragma once

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class MetadataComponent;
class ShaderWrapper;

class ColorComponent {
public:
    static nanogui::Vector4f glm_to_nanogui(glm::vec3 color){
		return nanogui::Vector4f(color.x, color.y, color.z, 1.0f);
    }
	
	ColorComponent(MetadataComponent& metadataComponent, ShaderWrapper& shaderWrapper);

	void set_color(const glm::vec4& color);

	void apply();

	glm::vec4 get() const {
		return mColor;
	}
	
	MetadataComponent& mMetadataComponent;
	ShaderWrapper& mShader;
	glm::vec4 mColor;
};
