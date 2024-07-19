#pragma once

#include <nanogui/shader.h>
#include <string>
#include <unordered_map>
#include <memory>

class ShaderManager {
public:
	ShaderManager(nanogui::RenderPass *render_pass);
	
	nanogui::ref<nanogui::Shader> load_shader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path);
	nanogui::ref<nanogui::Shader> get_shader(const std::string &name);
	
private:
	std::unordered_map<std::string, nanogui::ref<nanogui::Shader>> mShaderCache;
	nanogui::RenderPass *mRenderPass;
	
	std::string read_file(const std::string &file_path);
	void load_default_shaders();
};
