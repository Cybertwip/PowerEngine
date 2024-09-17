#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/shader.h>

ShaderWrapper::ShaderWrapper(nanogui::Shader& shader) : mShader(shader) {}

void ShaderWrapper::set_buffer(const std::string &name, nanogui::VariableType type,
							   std::initializer_list<size_t> shape, const void *data, int index) {
	mShader.set_buffer(name, type, shape.end() - shape.begin(), shape.begin(), data, index);
}

void ShaderWrapper::set_texture(const std::string& name, nanogui::Texture& texture) {
	mShader.set_texture(name, &texture);
}
void ShaderWrapper::begin() { mShader.begin(); }
void ShaderWrapper::end() { mShader.end(); }
void ShaderWrapper::draw_array(nanogui::Shader::PrimitiveType primitive_type, size_t offset,
							   size_t count, bool indexed) {
	mShader.draw_array(primitive_type, offset, count, indexed);
}
