#pragma once

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class MetadataComponent;
class ShaderWrapper;

class ColorComponent {
public:
    static nanogui::Vector3f glm_to_nanogui(glm::vec3 color){
		return nanogui::Vector3f(color.x, color.y, color.z);
    }
	
	ColorComponent(MetadataComponent& metadataComponent, ShaderWrapper& shaderWrapper);

	void set_color(const glm::vec3& color);

	void apply();

	glm::vec3 get() const {
		return mColor;
	}
	
	MetadataComponent& mMetadataComponent;
	ShaderWrapper& mShader;
	glm::vec3 mColor;
};
