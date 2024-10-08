#pragma once

#include <nanogui/shader.h>

#include <memory>
#include <string>
#include <unordered_map>

class Canvas;

class ShaderManager
{
   public:
    ShaderManager(nanogui::Canvas& canvas);

    std::shared_ptr<nanogui::Shader> get_shader(const std::string &name);

	std::shared_ptr<nanogui::RenderPass> render_pass() const {
		return mRenderPass;
	}
	
	int identifier(const std::string& name);
	void load_default_shaders();
	std::shared_ptr<nanogui::Shader> load_shader(const std::string &name,
											  const std::string &vertex_path,
											  const std::string &fragment_path, nanogui::Shader::BlendMode blendMode = nanogui::Shader::BlendMode::None);
   private:
    std::unordered_map<std::string, std::shared_ptr<nanogui::Shader>> mShaderCache;
	nanogui::RenderPass& mRenderPass;

    std::string read_file(const std::string &file_path);
};
