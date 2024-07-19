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
	
	void upload_index_data(const std::vector<uint32_t>& index_data);	

	template <typename Array> void set_uniform(const std::string &name,
															  const Array &value) {
		mShader.set_uniform(name, value);
	}

	void set_texture(const std::string &name, nanogui::Texture *texture);
	void begin();
	void end();
	void draw_array(nanogui::Shader::PrimitiveType primitive_type,
					size_t offset, size_t count,
					bool indexed = false);

protected:
	nanogui::Shader& mShader;
};
