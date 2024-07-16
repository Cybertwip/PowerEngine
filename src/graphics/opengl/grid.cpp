#include "grid.h"
#include <glad/glad.h>

namespace anim
{
Grid::Grid()
{
	setup_grid_buffers();
}

Grid::~Grid()
{
	glDeleteVertexArrays(1, &grid_VAO_);
	glDeleteBuffers(1, &grid_VBO_);
}

void Grid::draw(Shader &shader, anim::CameraComponent &camera)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	shader.use();
	
	// Set the view and projection matrices
	shader.set_mat4("view", camera.get_view());
	shader.set_mat4("projection", camera.get_projection());
	
	// Pass the near and far plane distances to the fragment shader

	float u_nearfar[2] = { 1.0f, 5e3f };
	shader.set_float_array("u_nearfar", u_nearfar, 2);
	
	glBindVertexArray(grid_VAO_);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}


void Grid::setup_grid_buffers()
{
	float quadVertices[] = {
		// positions
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f,
		1.0f,  1.0f,
		-1.0f,  1.0f,
		-1.0f, -1.0f
	};
	
	glGenVertexArrays(1, &grid_VAO_);
	glGenBuffers(1, &grid_VBO_);
	
	glBindVertexArray(grid_VAO_);
	
	glBindBuffer(GL_ARRAY_BUFFER, grid_VBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}
}
