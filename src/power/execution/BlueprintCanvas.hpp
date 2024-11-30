#pragma once

#include "Canvas.hpp"

#include <nanogui/button.h>
#include <nanogui/popup.h>

#include <optional>
#include <functional>

class ScenePanel;
class ShaderManager;
class Grid2d;

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
	
	void clear();
	
	void add_link(Link* link);
	
	BlueprintNode* selected_node() {
		return mSelectedNode;
	}
	
	void clear_selection() {
		mSelectedNode = nullptr;
	}
	
private:
	// Override mouse_button_event to consume the event
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
	void draw(NVGcontext *ctx) override;
	
	bool query_link(Pin& source_pin, Pin& destination_pin);
	
	void setup_options();
	
private:
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	std::unique_ptr<nanogui::Popup> mContextMenu;
	std::vector<std::unique_ptr<nanogui::Button>> mNodeOptions;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	bool mScrolling;
	
	NodeProcessor& mNodeProcessor;
	
	std::vector<BlueprintNode*> mNodes;
	BlueprintNode* mSelectedNode;
	std::vector<Link*> mLinks;

	nanogui::Vector2i mMousePosition;
	int mHeaderHeight;
	std::optional<std::reference_wrapper<Pin>> mActiveOutputPin;
	std::optional<std::reference_wrapper<Pin>> mActiveInputPin;
};
