#pragma once

#include "components/MetadataComponent.hpp"

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
			   std::initializer_list<size_t> shape, const void *data, int index = -1);
	
	template <typename Array> void set_uniform(const std::string &name,
															  const Array &value) {
		mShader.set_uniform(name, value);
	}

	void set_texture(const std::string &name, nanogui::Texture& texture, int index = 0);
	
	size_t get_buffer_size(const std::string& name);
	
	int identifier() const;

	void begin();
	void end();
	void draw_array(nanogui::Shader::PrimitiveType primitive_type,
					size_t offset, size_t count,
					bool indexed = false);
	
	nanogui::RenderPass& render_pass() const {
		return *mShader.render_pass();
	}

protected:
	nanogui::Shader& mShader;
	
private:
	MetadataComponent mMetadata;
};
