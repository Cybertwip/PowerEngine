#include "gl_sprite.h"
#include <glad/glad.h>
#include "shader.h"

namespace anim::gl
{
GLSprite::GLSprite(const std::string& path, const MaterialProperties& mat_properties)
: Sprite(path, mat_properties)
{
	// Define the vertices for a quad (two triangles)
	vertices_ = {
		Vertex({-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}),
		Vertex({0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}),
		Vertex({0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}),
		Vertex({-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f})
	};
	
	// Define the indices for the quad
	indices_ = {0, 1, 2, 2, 3, 0};
	
	init_buffer();
}

GLSprite::~GLSprite()
{
	glDeleteVertexArrays(1, &VAO_);
	glDeleteBuffers(1, &VBO_);
	glDeleteBuffers(1, &EBO_);
}
void GLSprite::draw_sprite(Shader& shader)
{
	shader.use();
	
	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_.id);
	shader.set_int("texture1", 0);
	
	// Draw sprite
	glBindVertexArray(VAO_);
	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices_.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
void GLSprite::draw(anim::Shader& shader)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_REPLACE, GL_REPLACE);
	glStencilMask(0xFF);
	
	draw_sprite(shader);
	
	glDisable(GL_STENCIL_TEST);
	
}

void GLSprite::draw_shadow(anim::Shader &shader){
	glBindVertexArray(VAO_);
	
	if (indices_.size() > 0)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices_.size()), GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO immediately after the draw call
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
	}
	
	glBindVertexArray(0);
}

void GLSprite::draw_outline(anim::Shader& shader)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 2, 0xFF);
	glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_REPLACE, GL_REPLACE);
	glStencilMask(0xFF);
	
	draw_sprite(shader);
	
	glDisable(GL_STENCIL_TEST);
}


void GLSprite::init_buffer()
{
	glGenVertexArrays(1, &VAO_);
	glGenBuffers(1, &VBO_);
	glGenBuffers(1, &EBO_);
	
	glBindVertexArray(VAO_);
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);
	
	// Vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords1));
	
	glBindVertexArray(0);
}

void GLSprite::update_vertices(const std::vector<Vertex>& newVertices)
{
	vertices_ = newVertices; // Update the local vertex store if necessary
	glBindVertexArray(VAO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_DYNAMIC_DRAW);
}


}
