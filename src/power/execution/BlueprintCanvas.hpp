#pragma once

#include "Canvas.hpp"
#include "serialization/UUID.hpp"

#include <nanogui/button.h>
#include <nanogui/popup.h>

#include <optional>
#include <functional>

class ScenePanel;
class ShaderManager;
class Grid2d;

class Link;
class CoreNode;
class CorePin;
class VisualPin;
class NodeProcessor;
class VisualBlueprintNode;
class VisualLink;

class BlueprintCanvas : public Canvas {
public:
	BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, NodeProcessor& nodeProcessor, std::function<void()> onCanvasModifiedCallback, nanogui::Color backgroundColor);
	
	void on_output_pin_clicked(VisualPin& pin);
	void on_input_pin_clicked(VisualPin& pin);
	
	void draw();
	
	void clear();
	
	void attach_popup();

	void add_link(VisualPin& start, VisualPin& end);

	void link(CorePin& start, CorePin& end);

	VisualBlueprintNode* selected_node() {
		return mSelectedNode;
	}
	
	void clear_selection() {
		mSelectedNode = nullptr;
	}
	
	
	template<typename T, typename U>
	void spawn_node(nanogui::Vector2i position, U& coreNode) {
		auto node = std::make_unique<T>(*this, position, nanogui::Vector2i(196, 244), coreNode);
		mVisualNodes.push_back(std::move(node));
		mOnCanvasModifiedCallback();
	}
	
	void on_modified() {
		if (mOnCanvasModifiedCallback) {
			mOnCanvasModifiedCallback();
		}
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
private:
	// Override mouse_button_event to consume the event
	// Delegation is done via the passthrough widgets
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	
	void draw(NVGcontext *ctx) override;
	
	bool query_link(VisualPin& source_pin, VisualPin& destination_pin);
	
	void setup_options();
	
	VisualBlueprintNode* find_node(UUID id);

private:
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	static std::unique_ptr<nanogui::Popup> SContextMenu;
	std::vector<std::unique_ptr<nanogui::Button>> mNodeOptions;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	bool mScrolling;
	
	NodeProcessor& mNodeProcessor;
	std::function<void()> mOnCanvasModifiedCallback;

	VisualBlueprintNode* mSelectedNode;
	
	std::vector<std::unique_ptr<VisualBlueprintNode>> mVisualNodes;
	
	std::vector<std::unique_ptr<VisualLink>> mLinks;

	nanogui::Vector2i mMousePosition;
	
	int mHeaderHeight;
	std::optional<std::reference_wrapper<VisualPin>> mActiveOutputPin;
	std::optional<std::reference_wrapper<VisualPin>> mActiveInputPin;
};
