#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/shader.h>

ShaderWrapper::ShaderWrapper(nanogui::Shader& shader) : mShader(shader), mMetadata(std::hash<std::string>{}(shader.name()), shader.name())
 {}

void ShaderWrapper::set_buffer(const std::string &name,
		   nanogui::VariableType dtype,
		   size_t ndim,
		   const size_t *shape,
		   const void *data,
							   int index) {
	mShader.set_buffer(name, dtype, ndim, shape, data, index);
}

void ShaderWrapper::set_buffer(const std::string &name, nanogui::VariableType type,
							   std::initializer_list<size_t> shape, const void *data, int index) {
	mShader.set_buffer(name, type, shape.end() - shape.begin(), shape.begin(), data, index);
}

void ShaderWrapper::set_texture(const std::string& name, nanogui::Texture& texture, int index) {
	mShader.set_texture(name, &texture, index);
}

size_t ShaderWrapper::get_buffer_size(const std::string& name) {
	return mShader.get_buffer_size(name);
}

int ShaderWrapper::identifier() const {
	return mMetadata.identifier();
}

void ShaderWrapper::begin() { mShader.begin(); }
void ShaderWrapper::end() { mShader.end(); }
void ShaderWrapper::draw_array(nanogui::Shader::PrimitiveType primitive_type, size_t offset,
							   size_t count, bool indexed) {
	mShader.draw_array(primitive_type, offset, count, indexed);
}
