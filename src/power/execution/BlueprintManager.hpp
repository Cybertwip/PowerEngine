#pragma once

#include "Canvas.hpp"

#include "components/TransformComponent.hpp"
#include "graphics/drawing/Grid.hpp"
#include "simulation/SimulationServer.hpp"
#include "ShaderManager.hpp"

#include <nanogui/icons.h>
#include <nanogui/renderpass.h>
#include <nanogui/toolbutton.h>

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <variant>
#include <optional>
#include <iostream>

namespace blueprint {

enum class PinType {
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

enum class PinSubType {
	None,
	Actor,
	Light,
	Camera,
	Animation,
	Sequence,
	Composition
};

enum class PinKind {
	Output,
	Input
};

struct Node;
struct Link;

struct Entity {
	int id;
	std::optional<std::variant<std::string, int, float, bool>> payload;
};

struct Pin {
	int id;
	int node_id;
	Node* node;
	std::string name;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	std::optional<std::variant<Entity, std::string, int, float, bool>> data;
	
	Pin(int id, int node_id, const char* name, PinType type, PinSubType subtype = PinSubType::None)
	: id(id), node_id(node_id), node(nullptr), name(name), type(type), subtype(subtype), kind(PinKind::Input) {
		if (type == PinType::Bool) {
			data = true;
		} else if (type == PinType::String) {
			data = "";
		} else if (type == PinType::Float) {
			data = 0.0f;
		} else if (type == PinType::Flow) {
			data = true;
		}
	}
};

struct Node {
	int id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	nanogui::Color color;
	nanogui::Vector2i size;
	bool root_node = false;
	std::function<void()> on_linked;
	std::function<void()> evaluate;
	
	Node(int id, const char* name, nanogui::Color color = nanogui::Color(255, 255, 255, 255))
	: id(id), name(name), color(color), size(nanogui::Vector2i(0, 0)) {}
	
	void reset_flow() {
		for (auto& input : inputs) {
			if (input.type == PinType::Flow) {
				input.can_flow = false;
			}
		}
		for (auto& output : outputs) {
			if (output.type == PinType::Flow) {
				output.can_flow = false;
			}
		}
	}
};

struct Link {
	int id;
	int start_pin_id;
	int end_pin_id;
	nanogui::Color color;
	
	Link(int id, int start_pin_id, int end_pin_id)
	: id(id), start_pin_id(start_pin_id), end_pin_id(end_pin_id), color(nanogui::Color(255, 255, 255, 255)) {}
};

class NodeProcessor {
public:
	NodeProcessor() {}
	
	int get_next_id() {
		return next_id++;
	}
	
	void build_node(Node* node) {
		for (auto& input : node->inputs) {
			input.node = node;
			input.kind = PinKind::Input;
		}
		for (auto& output : node->outputs) {
			output.node = node;
			output.kind = PinKind::Output;
		}
	}
	
	Node* spawn_input_action_node(const std::string& key_string, int key_code) {
		auto label = "Input Action " + key_string;
		auto node = std::make_unique<Node>(get_next_id(), label.c_str(), nanogui::Color(255, 128, 128, 255));
		node->root_node = true;
		node->outputs.emplace_back(get_next_id(), node->id, "Pressed", PinType::Flow);
		node->outputs.emplace_back(get_next_id(), node->id, "Released", PinType::Flow);
		
		node->evaluate = [node = node.get(), key_code]() {
			if (key_code != -1) {
				if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_PRESS) {
					node->outputs[0].can_flow = true;
				} else if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_RELEASE) {
					if (node->outputs[0].can_flow) {
						node->outputs[0].can_flow = false;
						node->outputs[1].can_flow = true;
					} else {
						node->outputs[1].can_flow = false;
					}
				}
			} else {
				node->outputs[0].can_flow = false;
				node->outputs[1].can_flow = false;
			}
		};
		
		build_node(node.get());
		nodes.push_back(std::move(node));
		return nodes.back().get();
	}
	
	void evaluate(Node* node) {
		if (node->evaluate) {
			node->evaluate();
		}
	}
	
	// Add more functions to define other nodes, and manage linking, deletion, etc.
	// ...
	
private:
	int next_id = 1;
	std::vector<std::unique_ptr<Node>> nodes;
	std::vector<std::unique_ptr<Link>> links;
	
	// Additional methods and logic needed for the complete functionality go here
	
};

}

#include "ui/ScenePanel.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <nanogui/nanogui.h>

class StringNode : public nanogui::Window {
public:
	StringNode(nanogui::Widget& parent, const std::string& title, nanogui::Vector2i size)
	: nanogui::Window(parent, title) {
		set_fixed_size(size);
		
		// Main window layout: Horizontal orientation, fill space
		set_layout(std::make_unique<nanogui::GroupLayout>(0, 0));
		
		// Left column placeholder (no pins)
		mLeftColumn = std::make_unique<nanogui::Widget>(*this);
		mLeftColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 15));
		
		// Middle column for the node data
		mDataColumn = std::make_unique<nanogui::Widget>(*this);
		mDataColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 15));
		
		// Right column for output pins: Aligned to the right edge
		mRightColumn = std::make_unique<nanogui::Widget>(*this);
		mRightColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Maximum, 0, 0));
		
		// Text box inside the node data wrapper: Will be centered due to the vertical alignment
		mTextBox = std::make_unique<nanogui::TextBox>(*mDataColumn, "");
		mTextBox->set_editable(true);
		
		// Output pin inside the output pin wrapper: Positioned on the right
		mOutputPin = std::make_unique<nanogui::ToolButton>(*mRightColumn, FA_CIRCLE_NOTCH);
		
		// Optional change callback for the output pin
		mOutputPin->set_change_callback([this](bool active) {
			if (active) {
				mOutputPin->set_icon(FA_CIRCLE);
			} else {
				mOutputPin->set_icon(FA_CIRCLE_NOTCH);
			}
		});
		
		mRightColumn->set_position(fixed_width() - mOutputPin->width());
	}
	
private:
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
	std::unique_ptr<nanogui::TextBox> mTextBox;
	std::unique_ptr<nanogui::ToolButton> mOutputPin;
};

class BlueprintPanel : public ScenePanel {
public:
	BlueprintPanel(Canvas& parent)
	: ScenePanel(parent, "Blueprint")
	, mScrollX(0)
	, mScrollY(0) {
		// Set the layout to horizontal with some padding
		set_layout(std::make_unique<nanogui::GroupLayout>(0, 0, 0));
			
		// Ensure the canvas is created with the correct dimensions or set it explicitly if needed
		mCanvas = std::make_unique<Canvas>(*this, parent.screen(), nanogui::Color(35, 65, 90, 255));
		
		mCanvas->set_fixed_size(nanogui::Vector2i(fixed_width(), parent.fixed_height() * 0.71));

		mShaderManager = std::make_unique<ShaderManager>(*mCanvas);
		
		mGrid = std::make_unique<Grid2d>(*mShaderManager);
		
		// Register draw callback
		mCanvas->register_draw_callback([this]() {
			this->draw();
		});
		
		// Adjusted orthographic projection parameters
		float left = -1.0f;
		float right = 1.0f;
		float bottom = -1.0f;
		float top = 1.0f;
		float near = -1.0f;
		float far = 1.0f;
		
		mProjection = nanogui::Matrix4f::ortho(left, right, bottom, top, near, far);

		mView = nanogui::Matrix4f::look_at(
										   nanogui::Vector3f(0, 0, 1),  // Camera position
										   nanogui::Vector3f(0, 0, 0),  // Look-at point
										   nanogui::Vector3f(0, 1, 0)   // Up direction
										   );
		
		register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
			
			// because dx and dy are global screen space and its range is -0.5 to 0.5
			float scaledDx = dx * (width / mCanvas->fixed_width()) * 2.0f;
			float scaledDy = dy * (height / mCanvas->fixed_height()) * 2.0f;
			
			mScrollX += scaledDx;
			mScrollY += scaledDy;
			
			mGrid->set_scroll_offset(nanogui::Vector2i(-mScrollX, -mScrollY));

			// Assuming mTestWidget's position is in screen space, update it
			nanogui::Vector2i currentPos = mTestWidget->position();
			mTestWidget->set_position(currentPos + nanogui::Vector2i(dx, dy));
		});
		
		
		mTestWidget = std::make_unique<StringNode>(*mCanvas, "Test", nanogui::Vector2i(144, 164));
		
		mTestWidget->set_position(nanogui::Vector2i(0, 0));
	}
	
private:
	void draw() {
		mCanvas->render_pass().clear_color(0, mCanvas->background_color());
		mGrid->draw_content(nanogui::Matrix4f::identity(), mView, mProjection);
	}
	
	void draw(NVGcontext *ctx) override {
		ScenePanel::draw(ctx);
	}
	
private:
	std::unique_ptr<Canvas> mCanvas;
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	std::unique_ptr<StringNode> mTestWidget;
};

class BlueprintManager {
public:
	BlueprintManager(Canvas& canvas) : mCanvas(canvas){
		mBlueprintPanel = std::make_shared<BlueprintPanel>(canvas);
		
		mBlueprintPanel->set_position(nanogui::Vector2i(0, canvas.fixed_height()));
		
		mBlueprintButton = std::make_shared<nanogui::ToolButton>(canvas, FA_FLASK);

		mBlueprintButton->set_fixed_size(nanogui::Vector2i(48, 48));
		
		mBlueprintButton->set_text_color(nanogui::Color(135, 206, 235, 255));

		// Position the button in the lower-right corner
		mBlueprintButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() * 0.5f - mBlueprintButton->fixed_width() * 0.5f, mCanvas.fixed_height() - mBlueprintButton->fixed_height() - 20));

		mBlueprintButton->set_change_callback([this](bool active) {
			toggle_blueprint_panel(active);
		});
	}
	
	
	void toggle_blueprint_panel(bool active) {
		if (mAnimationFuture.valid() && mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			return; // Animation is still running, do not start a new one
		}
		
		if (active) {
			mAnimationFuture = std::async(std::launch::async, [this]() {
				auto target = nanogui::Vector2i(0, mCanvas.fixed_height() * 0.25f);
				animate_panel_position(target);
			});
		} else {
			mAnimationFuture = std::async(std::launch::async, [this]() {
				auto target = nanogui::Vector2i(0, mCanvas.fixed_height());
				animate_panel_position(target);
			});
		}

		mBlueprintPanel->set_visible(true);
	}
	
	void animate_panel_position(const nanogui::Vector2i &targetPosition) {
		const int steps = 20;
		const auto stepDelay = std::chrono::milliseconds(16); // ~60 FPS
		nanogui::Vector2f startPos = mBlueprintPanel->position();
		nanogui::Vector2f step = (nanogui::Vector2f(targetPosition) - startPos) / (steps - 1);
		
		for (int i = 0; i < steps; ++i) {
			// This check ensures we can stop the animation if another toggle happens
			if (mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				mBlueprintPanel->set_position(startPos + step * i);
				std::this_thread::sleep_for(stepDelay);
			} else {
				break; // Stop animation if future is no longer valid (another animation started)
			}
		}
	}
	
	void process_events() {
		mBlueprintPanel->process_events();
	}
	
private:
	Canvas& mCanvas;
	std::shared_ptr<BlueprintPanel> mBlueprintPanel;
	std::shared_ptr<nanogui::ToolButton> mBlueprintButton;
	std::future<void> mAnimationFuture;

};
