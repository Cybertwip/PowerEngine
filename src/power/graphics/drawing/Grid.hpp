#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include "glm/glm.hpp"

#include <vector>

class ShaderManager;

class Grid : public Drawable
{
public:
	Grid(ShaderManager& shaderManager);
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
private:
	ShaderWrapper mShaderWrapper;
	std::vector<float> mGridVertices;
	std::vector<unsigned int> mGridIndices;
};


class Grid2d : public Drawable
{
public:
	Grid2d(ShaderManager& shaderManager);
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
	
	void set_scroll_offset(const nanogui::Vector2f& offset);
	
private:
	ShaderWrapper mShaderWrapper;
	std::vector<float> mGridVertices;
	nanogui::Vector2f mScrollOffset;
	float mGridSize;
	float mLineWidth;
};
