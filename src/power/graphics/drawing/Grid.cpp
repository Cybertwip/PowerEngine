#include "Grid.hpp"
#include "ShaderManager.hpp"

#include "components/TransformComponent.hpp"

#include <glm/gtc/matrix_transform.hpp>


#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
#include <nanogui/opengl.h>
#elif defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif
Grid::Grid(ShaderManager& shaderManager)
: mShaderWrapper(shaderManager.get_shader("grid"))
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

void Grid::draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) {
	// API-specific setup for depth testing and blending
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	// OpenGL: Enable depth test and blending
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
#endif
	
	// Common code: Set up vertex buffer
	mShaderWrapper.set_buffer("aPosition", nanogui::VariableType::Float32, {mGridVertices.size() / 2, 2}, mGridVertices.data());
	
	// Common code: Near and far planes
	float nearPlane = 0.01f;   // Example value for near plane
	float farPlane = 5e3f;      // Example value for far plane
	
	// Common code: Set uniforms
	mShaderWrapper.set_uniform("u_near", nearPlane);
	mShaderWrapper.set_uniform("u_far", farPlane);
	mShaderWrapper.set_uniform("aView", view);
	mShaderWrapper.set_uniform("aProjection", projection);
	
	nanogui::Matrix4f projectionView = projection * view;
	
	nanogui::Matrix4f projectionViewInverse = projectionView.inverse();

	mShaderWrapper.set_uniform("aInvProjectionView", projectionViewInverse); // Pass inverse matrix

	
	// Common code: Begin shader, draw, and end
	mShaderWrapper.begin();
	mShaderWrapper.draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, false);
	mShaderWrapper.end();
	
	// API-specific cleanup for depth testing (if needed)
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
	glDisable(GL_DEPTH_TEST);
#endif
}
