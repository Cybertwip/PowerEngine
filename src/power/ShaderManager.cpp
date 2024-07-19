#include "ShaderManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

std::string ShaderManager::read_file(const std::string &file_path) {
	std::ifstream file(file_path);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + file_path);
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

ShaderManager::ShaderManager(nanogui::RenderPass *render_pass) : mRenderPass(render_pass) {
	//load_default_shaders();
}

nanogui::ref<nanogui::Shader> ShaderManager::load_shader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path) {
	if (mShaderCache.find(name) != mShaderCache.end()) {
		return mShaderCache[name];
	}
	
	std::string vertex_code = read_file(vertex_path);
	std::string fragment_code = read_file(fragment_path);
	nanogui::ref<nanogui::Shader> shader = new nanogui::Shader(mRenderPass, name, vertex_code, fragment_code);
	mShaderCache[name] = shader;
	return shader;
}

nanogui::ref<nanogui::Shader> ShaderManager::get_shader(const std::string &name) {
	if (mShaderCache.find(name) != mShaderCache.end()) {
		return mShaderCache[name];
	}
	throw std::runtime_error("Shader not found: " + name);
}

void ShaderManager::load_default_shaders() {
	// Add the shader file paths here
	load_shader("animation_skinning", "shaders/animation_skinning.vs", "shaders/animation_skinning.fs");
	load_shader("armature", "shaders/armature.vs", "shaders/armature.fs");
	load_shader("grid", "shaders/grid.vs", "shaders/grid.fs");
	load_shader("outline", "shaders/outline.vs", "shaders/outline.fs");
	load_shader("phong_model", "shaders/phong_model.vs", "shaders/phong_model.fs");
	load_shader("shadow", "shaders/shadow.vs", "shaders/shadow.fs");
	load_shader("simple_framebuffer", "shaders/simple_framebuffer.vs", "shaders/simple_framebuffer.fs");
	load_shader("simple_model", "shaders/simple_model.vs", "shaders/simple_model.fs");
	load_shader("sprite_billboard", "shaders/sprite_billboard.vs", "shaders/sprite_billboard.fs");
	load_shader("ui_model", "shaders/ui_model.vs", "shaders/ui_model.fs");
}
