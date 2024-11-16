#pragma once

#include <nanogui/screen.h>

#include "Canvas.hpp"

#include <optional>
#include <functional>

class ScenePanel;
class ShaderManager;
class Grid2d;

namespace blueprint {
class Node;
class Pin;

class BlueprintCanvas : public Canvas {
public:
	BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, nanogui::Color backgroundColor);
	
	void add_node(Node* node);
	
	void on_output_pin_clicked(Pin& pin);
	
	void draw();
	
private:
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
	void draw(NVGcontext *ctx) override;
	
private:
	
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	std::vector<blueprint::Node*> mNodes;
	
	nanogui::Vector2i mMousePosition;
	int mHeaderHeight;
	std::optional<std::reference_wrapper<Pin>> mActiveOutputPin;
};

}
