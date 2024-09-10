#pragma once

#include <nanogui/shader.h>
#include <nanogui/texture.h>

#include <string>

namespace nanogui{
class Shader;
}

class ShaderWrapper
{
public:
	ShaderWrapper(nanogui::Shader& shader);
	
	void set_buffer(const std::string &name, nanogui::VariableType type,
			   std::initializer_list<size_t> shape, const void *data);
	
	template <typename Array> void set_uniform(const std::string &name,
															  const Array &value) {
		mShader.set_uniform(name, value);
	}

	void set_texture(const std::string &name, nanogui::Texture& texture);
	void begin();
	void end();
	void draw_array(nanogui::Shader::PrimitiveType primitive_type,
					size_t offset, size_t count,
					bool indexed = false);

protected:
	nanogui::Shader& mShader;
};
