#include "ShaderManager.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem> // For std::filesystem (C++17 and later)
#include <mach-o/dyld.h> // For _NSGetExecutablePath (macOS-specific)

namespace fs = std::filesystem;

// Helper function to get the path to the Resources directory
std::string get_resources_path() {
	char path[1024];
	uint32_t size = sizeof(path);
	if (_NSGetExecutablePath(path, &size) != 0) {
		throw std::runtime_error("Failed to get executable path: buffer too small");
	}
	
	// Convert to filesystem path and resolve to Resources directory
	fs::path exe_path(path);
	fs::path resources_path = exe_path.parent_path().parent_path() / "Resources";
	
	// Normalize the path and check if it exists
	if (!fs::exists(resources_path)) {
		throw std::runtime_error("Resources directory not found at: " + resources_path.string());
	}
	
	return resources_path.string();
}

std::string ShaderManager::read_file(const std::string &file_path) {
	// Get the Resources directory path
	static std::string resources_base_path = get_resources_path();
	
	// Construct the full path
	fs::path full_path = fs::path(resources_base_path) / file_path;
	
	std::ifstream file(full_path.string());
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + full_path.string());
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

ShaderManager::ShaderManager(nanogui::Canvas& canvas) : mRenderPass(canvas.render_pass()) {
}

std::shared_ptr<nanogui::Shader> ShaderManager::load_shader(
															const std::string &name,
															const std::string &vertex_path,
															const std::string &fragment_path,
															nanogui::Shader::BlendMode blendMode
															) {
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
	auto it = mShaderCache.find(name);
	if (it != mShaderCache.end()) {
		return it->second;
	}
	throw std::runtime_error("Shader not found: " + name);
}

void ShaderManager::load_default_shaders() {
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	load_shader("mesh", "internal/shaders/gl/diffuse.vs", "internal/shaders/gl/diffuse.fs");
	load_shader("gizmo", "internal/shaders/gl/gizmo.vs", "internal/shaders/gl/gizmo.fs");
	load_shader("grid", "internal/shaders/gl/grid.vs", "internal/shaders/gl/grid.fs");
#elif defined(NANOGUI_USE_METAL)
	load_shader("mesh", "internal/shaders/metal/diffuse_vs.metal", "internal/shaders/metal/diffuse_fs.metal",
				nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("skinned_mesh", "internal/shaders/metal/diffuse_skinned_vs.metal",
				"internal/shaders/metal/diffuse_fs.metal", nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("gizmo", "internal/shaders/metal/gizmo_vs.metal", "internal/shaders/metal/gizmo_fs.metal",
				nanogui::Shader::BlendMode::AlphaBlend);
	load_shader("grid", "internal/shaders/metal/grid_vs.metal", "internal/shaders/metal/grid_fs.metal",
				nanogui::Shader::BlendMode::AlphaBlend);
#endif
}

int ShaderManager::identifier(const std::string& name) {
	return std::hash<std::string>{}(get_shader(name)->name());
}
