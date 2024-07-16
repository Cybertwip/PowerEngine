#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "mesh.h"

class Scene;

namespace anim
{
class Shader;
struct Texture;
struct MaterialProperties;
struct Vertex;

class Sprite
{
public:
	Sprite(const std::string& path, const MaterialProperties &mat_properties);
	virtual ~Sprite() = default;
	virtual void draw(Shader &shader) = 0;
	virtual void draw_shadow(Shader &shader) = 0;
	virtual void draw_outline(Shader &shader) = 0;
	
	MaterialProperties &get_mutable_mat_properties()
	{
		return mat_properties_;
	}
	
	const Texture& get_texture() const {
		return texture_;
	}
	
	const std::vector<Vertex>& get_vertices() const {
		return vertices_;
	}
	
	const std::vector<unsigned int>& get_indices() const {
		return indices_;
	}
	
protected:
	void build_sprite_geometry();
	
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> indices_;
	Texture texture_;
	MaterialProperties mat_properties_;
};
}

