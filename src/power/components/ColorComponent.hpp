#pragma once

#include <glm/glm.hpp>

#include <nanogui/vector.h>

class ShaderWrapper;

class ColorComponent {
public:
    static nanogui::Vector3f glm_to_nanogui(glm::vec3 color){
		return nanogui::Vector3f(color.x, color.y, color.z);
    }
	
	ColorComponent(ShaderWrapper& shaderWrapper);

	void set_color(const glm::vec3& color);

	glm::vec3 get_color() const {
		return mColor;
	}
	
	ShaderWrapper& mShader;
	glm::vec3 mColor;
};
