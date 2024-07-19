#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/shader.h>

ShaderWrapper::ShaderWrapper(nanogui::Shader& shader)
: mShader(shader) {
	
}

void ShaderWrapper::upload_index_data(const std::vector<uint32_t> &index_data){
	mShader.set_buffer("indices", nanogui::VariableType::UInt32, {index_data.size()}, index_data.data());
}

void ShaderWrapper::set_texture(const std::string &name, nanogui::Texture *texture){
	mShader.set_texture(name, texture);
}
void ShaderWrapper::begin(){
	mShader.begin();
}
void ShaderWrapper::end(){
	mShader.end();
}
void ShaderWrapper::draw_array(nanogui::Shader::PrimitiveType primitive_type,
				size_t offset, size_t count,
				bool indexed){
	mShader.draw_array(primitive_type, offset, count, indexed);
}
