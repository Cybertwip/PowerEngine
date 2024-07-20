#pragma once

#include <nanogui/shader.h>
#include <string>
#include <unordered_map>
#include <memory>

class Canvas;

class ShaderManager {
public:
	ShaderManager(Canvas& canvas);

    nanogui::ref<nanogui::Shader> get_shader(const std::string &name);
	
private:
    nanogui::ref<nanogui::Shader> load_shader(const std::string &name, const std::string &vertex_path, const std::string &fragment_path);

	std::unordered_map<std::string, nanogui::ref<nanogui::Shader>> mShaderCache;
	nanogui::RenderPass& mRenderPass;
	
	std::string read_file(const std::string &file_path);
	void load_default_shaders();
};
