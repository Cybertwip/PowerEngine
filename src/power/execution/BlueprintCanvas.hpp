#pragma once

#include "Canvas.hpp"

#include <optional>
#include <functional>

class ScenePanel;
class ShaderManager;
class Grid2d;

namespace blueprint {
class Link;
class BlueprintNode;
class NodeProcessor;
class Pin;

class BlueprintCanvas : public Canvas {
public:
	BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, NodeProcessor& nodeProcessor, nanogui::Color backgroundColor);
	
	void add_node(BlueprintNode* node);
	
	void on_output_pin_clicked(Pin& pin);
	void on_input_pin_clicked(Pin& pin);
	
	void draw();
	
private:
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
	void draw(NVGcontext *ctx) override;
	
	bool query_link(Pin& source_pin, Pin& destination_pin);
	
private:
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	NodeProcessor& mNodeProcessor;
	
	std::vector<blueprint::BlueprintNode*> mNodes;
	std::vector<blueprint::Link*> mLinks;

	nanogui::Vector2i mMousePosition;
	int mHeaderHeight;
	std::optional<std::reference_wrapper<Pin>> mActiveOutputPin;
	std::optional<std::reference_wrapper<Pin>> mActiveInputPin;
};

}
