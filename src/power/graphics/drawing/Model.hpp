#pragma once

#include "Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>

#include <glm/glm.hpp>

#include <array>


class Model : public Drawable {
public:
	class Vertex {
	public:
		Vertex();
		Vertex(const glm::vec3 &pos, const glm::vec2 &tex);
	
		void set_position(const glm::vec3 &vec);
		void set_normal(const glm::vec3 &vec);
		void set_texture_coords1(const glm::vec2 &vec);
		void set_texture_coords2(const glm::vec2 &vec);
		
		glm::vec3 get_position() const;
		glm::vec3 get_normal() const;
		glm::vec2 get_tex_coords1() const;
		glm::vec2 get_tex_coords2() const;

	private:
		glm::vec3 mPosition;
		glm::vec3 mNormal;
		glm::vec2 mTexCoords1;
		glm::vec2 mTexCoords2;
	};

	class MeshShader : public ShaderWrapper {
	public:
		MeshShader(nanogui::Shader& shader);
		
		void upload_vertex_data(const std::vector<Vertex>& vertexData);
	};

public:
    Model(MeshShader& shader);
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
private:
	MeshShader& mShader;
	nanogui::Matrix4f mvp;
	void initialize_mesh();
};