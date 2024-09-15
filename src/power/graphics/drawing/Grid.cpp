#include "Grid.hpp"
#include "ShaderManager.hpp"

#include "components/TransformComponent.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <nanogui/opengl.h>

Grid::Grid(ShaderManager& shaderManager)
: mShaderWrapper(*shaderManager.get_shader("grid"))
{
	mGridVertices = {
		// positions
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f,
		1.0f,  1.0f,
		-1.0f,  1.0f,
		-1.0f, -1.0f
	};

}
void Grid::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection)
{
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	
	// Set up vertex buffer
	mShaderWrapper.set_buffer("aPosition", nanogui::VariableType::Float32, {mGridVertices.size() / 2, 2}, mGridVertices.data());
	
	float nearPlane = 0.01f;   // Example value for near plane
	float farPlane = 5e3; // Example value for far plane
	
	// Pass the near and far plane distances to the fragment shader
	mShaderWrapper.set_uniform("u_near", nearPlane);
	mShaderWrapper.set_uniform("u_far", farPlane);

	// Set shader uniforms
//	mShaderWrapper.set_uniform("color", nanogui::Vector3f{ 1.0f, 1.0f, 1.0f });
	mShaderWrapper.set_uniform("aView", view);
	mShaderWrapper.set_uniform("aProjection", projection);

	mShaderWrapper.begin();
	
	// Draw the grid
	mShaderWrapper.draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, false);
	
	mShaderWrapper.end();
}

