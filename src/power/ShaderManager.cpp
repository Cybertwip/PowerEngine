#include "ShaderManager.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Canvas.hpp"

std::string ShaderManager::read_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ShaderManager::ShaderManager(nanogui::Canvas& canvas) : mRenderPass(canvas.render_pass()) {
}

std::shared_ptr<nanogui::Shader> ShaderManager::load_shader(const std::string &name,
														 const std::string &vertex_path,
														const std::string &fragment_path, nanogui::Shader::BlendMode blendMode) {
	if (mShaderCache.find(name) != mShaderCache.end()) {
		return mShaderCache[name];
	}
	
	std::string vertex_code = read_file(vertex_path);
	std::string fragment_code = read_file(fragment_path);
	std::shared_ptr<nanogui::Shader> shader =
	std::make_shared<nanogui::Shader>(mRenderPass, name, vertex_code, fragment_code, blendMode);
	mShaderCache[name] = shader;
	return shader;
}

std::shared_ptr<nanogui::Shader> ShaderManager::get_shader(const std::string &name) {
    if (mShaderCache.find(name) != mShaderCache.end()) {
        return mShaderCache[name];
    }
    throw std::runtime_error("Shader not found: " + name);
}

void ShaderManager::load_default_shaders() {
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	load_shader("mesh", "internal/shaders/gl/diffuse.vs", "internal/shaders/gl/diffuse.fs");
	load_shader("gizmo", "internal/shaders/gl/gizmo.vs", "internal/shaders/gl/gizmo.fs");
	load_shader("grid", "internal/shaders/gl/grid.vs", "shaders/gl/grid.fs");
#elif defined(NANOGUI_USE_METAL)
	load_shader("mesh", "internal/shaders/metal/diffuse_vs.metal", "internal/shaders/metal/diffuse_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("skinned_mesh", "internal/shaders/metal/diffuse_skinned_vs.metal", "internal/shaders/metal/diffuse_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("gizmo", "internal/shaders/metal/gizmo_vs.metal", "internal/shaders/metal/gizmo_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("grid", "internal/shaders/metal/grid_vs.metal", "internal/shaders/metal/grid_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("grid2d", "internal/shaders/metal/grid2d_vs.metal", "internal/shaders/metal/grid2d_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
#endif
}

int ShaderManager::identifier(const std::string& name) {
	return std::hash<std::string>{}(get_shader(name)->name());
}
