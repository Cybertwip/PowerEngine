#pragma once

#include <nanogui/screen.h>

#include "Canvas.hpp"

class ScenePanel;
class ShaderManager;
class Grid2d;

namespace blueprint {
class Node;

class BlueprintCanvas : public Canvas {
public:
	BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, nanogui::Color backgroundColor);
	
	void add_node(Node* node);
	
	void draw();
	
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	std::vector<blueprint::Node*> mNodes;
};

}
