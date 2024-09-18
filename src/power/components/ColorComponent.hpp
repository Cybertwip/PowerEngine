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
	
	glm::vec4 get_color() const {
		return mColor;
	}
	
	void set_visible(bool visible);
	bool get_visible() const {
		return mVisible;
	}
	
	void apply();

	
private:
	MetadataComponent& mMetadataComponent;
	ShaderWrapper& mShader;
	glm::vec4 mColor;
	bool mVisible;
};
