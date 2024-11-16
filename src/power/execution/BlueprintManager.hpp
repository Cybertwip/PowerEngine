#pragma once

#include "Canvas.hpp"

#include "ShaderManager.hpp"

#include "components/TransformComponent.hpp"
#include "graphics/drawing/Grid.hpp"
#include "simulation/SimulationServer.hpp"
#include "ui/ScenePanel.hpp"

#include <nanogui/canvas.h>
#include <nanogui/icons.h>
#include <nanogui/textbox.h>
#include <nanogui/toolbutton.h>
#include <nanogui/renderpass.h>
#include <nanogui/vector.h>
#include <nanogui/window.h>

#include <nanovg.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

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

class Pin : public nanogui::ToolButton {
public:
	int id;
	int node_id;
	Node* node;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	std::optional<std::variant<Entity, std::string, int, float, bool>> data;
	
	Pin(nanogui::Widget& parent, int id, int node_id, const std::string& label, PinType type, PinSubType subtype = PinSubType::None)
	: nanogui::ToolButton(parent, FA_CIRCLE_NOTCH, label), id(id), node_id(node_id), node(nullptr), type(type), subtype(subtype), kind(PinKind::Input) {
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

class Node : public nanogui::Window {
public:
	int id;
	nanogui::Color color;
	bool root_node = false;
	std::function<void()> on_linked;
	std::function<void()> evaluate;
	
	Node(nanogui::Widget& parent, const std::string& name, nanogui::Vector2i size, int id, nanogui::Color color = nanogui::Color(255, 255, 255, 255))
	: nanogui::Window(parent, name), id(id), color(color) {
		set_fixed_size(size);
		
		set_layout(std::make_unique<nanogui::GroupLayout>(5, 5));

		mFlowContainer = std::make_unique<nanogui::Widget>(*this);

		// Left column inputs placeholder
		mLeftColumn = std::make_unique<nanogui::Widget>(*this);
		mLeftColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
		
		// Middle column for the node data
		mDataColumn = std::make_unique<nanogui::Widget>(*this);
		mDataColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
		
		// Right column for output pins: Aligned to the right edge
		mRightColumn = std::make_unique<nanogui::Widget>(*this);
		mRightColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
		
		mRightColumn->set_position(nanogui::Vector2i(fixed_size().x() - 48, 0));
	}

	template<typename T, typename... Args>
	T& add_data_widget(Args&&... args){
		auto data_widget = std::make_unique<T>(*mDataColumn, std::forward<Args>(args)...);
		
		data_widget->set_fixed_size(nanogui::Vector2i(fixed_size().x() - 48, 22));

		auto& data_widget_ref = *data_widget;

		data_widgets.push_back(std::move(data_widget));
		
		return data_widget_ref;
	}
	
	
	Pin& add_input(int pin_id, int node_id, const std::string& label, PinType pin_type) {
		
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mLeftColumn;
		
		auto input = std::make_unique<Pin>(parent, pin_id, node_id, label, pin_type);
		
		input->set_icon(pin_type == PinType::Flow ? FA_PLAY : FA_CIRCLE);
		
		auto& input_ref = *input;
		// Optional change callback for the output pin
		input->set_change_callback([&input_ref](bool active) {
			if (active) {
				input_ref.set_icon(FA_CIRCLE);
			} else {
				input_ref.set_icon(FA_CIRCLE_NOTCH);
			}
		});
		
		input->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (pin_type == PinType::Flow) {
			input->set_position(nanogui::Vector2i(5, 3));
		}

		inputs.push_back(std::move(input));
		
		return *inputs.back();
	}
	
	Pin& add_output(int pin_id, int node_id, const std::string& label, PinType pin_type) {
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mRightColumn;

		auto output = std::make_unique<Pin>(parent, pin_id, node_id, label, pin_type);
		
		if (pin_type == PinType::Flow) {
			output->set_icon(pin_type == PinType::Flow ? FA_PLAY : FA_CIRCLE);
		}

		auto& output_ref = *output;
		// Optional change callback for the output pin
		output->set_change_callback([&output_ref](bool active) {
			if (active) {
				output_ref.set_icon(FA_CIRCLE);
			} else {
				output_ref.set_icon(FA_CIRCLE_NOTCH);
			}
		});
		
		output->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (pin_type == PinType::Flow) {
			output->set_position(nanogui::Vector2i(fixed_size().x() - 22 - 5, 3));
		}

		outputs.push_back(std::move(output));
		
		return *outputs.back();
	}
	
	void build() {
		for (auto& input : inputs) {
			input->node = this;
			input->kind = PinKind::Input;
		}
		for (auto& output : outputs) {
			output->node = this;
			output->kind = PinKind::Output;
		}
	}
	
	void reset_flow() {
		for (auto& input : inputs) {
			if (input->type == PinType::Flow) {
				input->can_flow = false;
			}
		}
		for (auto& output : outputs) {
			if (output->type == PinType::Flow) {
				output->can_flow = false;
			}
		}
	}
	
protected:
	std::vector<std::unique_ptr<Pin>> inputs;
	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	std::vector<std::unique_ptr<Pin>> outputs;

private:
	
	void perform_layout(NVGcontext *ctx) override {
		Window::perform_layout(ctx);
		
		mFlowContainer->set_position(nanogui::Vector2i(5, 5));
	}
	void draw(NVGcontext *ctx) override {
		int ds = theme().m_window_drop_shadow_size, cr = theme().m_window_corner_radius;
		int hh = theme().m_window_header_height;
		
		/* Draw window */
		nvgSave(ctx);
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
		
		nvgFillColor(ctx, m_mouse_focus ? theme().m_window_fill_focused
					 : theme().m_window_fill_unfocused);
		nvgFill(ctx);
		
		
		/* Draw a drop shadow */
		NVGpaint shadow_paint = nvgBoxGradient(
											   ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr*2, ds*2,
											   theme().m_drop_shadow, theme().m_transparent);
		
		nvgSave(ctx);
		nvgResetScissor(ctx);
		nvgBeginPath(ctx);
		nvgRect(ctx, m_pos.x()-ds,m_pos.y()-ds, m_size.x()+2*ds, m_size.y()+2*ds);
		nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
		nvgPathWinding(ctx, NVG_HOLE);
		nvgFillPaint(ctx, shadow_paint);
		nvgFill(ctx);
		nvgRestore(ctx);
		
		if (!m_title.empty()) {
			/* Draw header */
			NVGpaint header_paint = nvgLinearGradient(
													  ctx, m_pos.x(), m_pos.y(), m_pos.x(),
													  m_pos.y() + hh,
													  theme().m_window_header_gradient_top,
													  theme().m_window_header_gradient_bot);
			
			nvgBeginPath(ctx);
			nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), hh, cr);
			
			nvgFillPaint(ctx, header_paint);
			nvgFill(ctx);
			
			nvgBeginPath(ctx);
			nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), hh, cr);
			nvgStrokeColor(ctx, theme().m_window_header_sep_top);
			
			nvgSave(ctx);
			nvgIntersectScissor(ctx, m_pos.x(), m_pos.y(), m_size.x(), 0.5f);
			nvgStroke(ctx);
			nvgRestore(ctx);
			
			nvgBeginPath(ctx);
			nvgMoveTo(ctx, m_pos.x() + 0.5f, m_pos.y() + hh - 1.5f);
			nvgLineTo(ctx, m_pos.x() + m_size.x() - 0.5f, m_pos.y() + hh - 1.5);
			nvgStrokeColor(ctx, theme().m_window_header_sep_bot);
			nvgStroke(ctx);
			
			nvgFontSize(ctx, 18.0f);
			nvgFontFace(ctx, "sans-bold");
			nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			
			nvgFontBlur(ctx, 2);
			nvgFillColor(ctx, theme().m_drop_shadow);
			nvgText(ctx, m_pos.x() + m_size.x() / 2,
					m_pos.y() + hh / 2, m_title.c_str(), nullptr);
			
			nvgFontBlur(ctx, 0);
			nvgFillColor(ctx, m_focused ? theme().m_window_title_focused
						 : theme().m_window_title_unfocused);
			nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + hh / 2 - 1,
					m_title.c_str(), nullptr);
		}
		
		nvgRestore(ctx);
		
		nvgTranslate(ctx, m_pos.x() - 5, m_pos.y() - 33);

		mFlowContainer->draw(ctx);
		
		nvgTranslate(ctx, -m_pos.x() - 5, -m_pos.y() + 33);
		
		nvgTranslate(ctx, m_pos.x(), m_pos.y());
		
		mColumnContainer->draw(ctx);
		
		nvgTranslate(ctx, -m_pos.x(), -m_pos.y());
		

	}

	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
};

struct Link {
	int id;
	int start_pin_id;
	int end_pin_id;
	nanogui::Color color;
	
	Link(int id, int start_pin_id, int end_pin_id)
	: id(id), start_pin_id(start_pin_id), end_pin_id(end_pin_id), color(nanogui::Color(255, 255, 255, 255)) {}
};


class PrintStringNode : public Node {
public:
	PrintStringNode(nanogui::Widget& parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id, int input_pin_id, int output_pin_id)
	: Node(parent, title, size, id) {
		auto& input_flow = add_input(flow_pin_id, this->id, "", PinType::Flow);
		auto& input = add_input(input_pin_id, this->id, "", PinType::String);
		auto& output_flow = add_output(output_pin_id, this->id, "", PinType::Flow);
		
		on_linked = [&output_flow](){
			output_flow.can_flow = true;
		};
		
		evaluate = [&input_flow, &input](){
			if(input_flow.can_flow){
				auto& data = input.data;
				if(data.has_value()){
					std::cout << std::get<std::string>(*data) << std::endl;
				}
			}
			
			input.can_flow = input_flow.can_flow;
		};
		

	}
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

class StringNode : public Node {
public:
	StringNode(nanogui::Widget& parent, const std::string& title, nanogui::Vector2i size, int id, int output_pin_id)
	: Node(parent, title, size, id) {
		
		// Text box inside the node data wrapper: Will be centered due to the vertical alignment
		auto& textbox = add_data_widget<nanogui::TextBox>("");
		
		textbox.set_editable(true);
		
		textbox.set_alignment(nanogui::TextBox::Alignment::Left);
		
		auto& output = add_output(output_pin_id, this->id, "", PinType::String);
				
		on_linked = [&output](){
			output.can_flow = true;
		};
	}
	
private:
	std::unique_ptr<nanogui::TextBox> mTextBox;
};

class BlueprintCanvas : public Canvas {
public:
	BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, nanogui::Color backgroundColor)
	: Canvas(parent, screen, backgroundColor)
	, mScrollX(0)
	, mScrollY(0) {
		
		mShaderManager = std::make_unique<ShaderManager>(*this);
		
		mGrid = std::make_unique<Grid2d>(*mShaderManager);
		
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
		
		parent.register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
			
			// because dx and dy are global screen space and its range is -0.5 to 0.5
			float scaledDx = dx * (width / fixed_width()) * 2.0f;
			float scaledDy = dy * (height / fixed_height()) * 2.0f;
			
			mScrollX += scaledDx;
			mScrollY += scaledDy;
			
			mGrid->set_scroll_offset(nanogui::Vector2i(-mScrollX, -mScrollY));
			
			// Assuming mTestWidget's position is in screen space, update it
			
			for (auto* node : mNodes) {
				nanogui::Vector2i currentPos = node->position();
				node->set_position(currentPos + nanogui::Vector2i(dx, dy));
			}
		});
		
		// Register draw callback
		register_draw_callback([this]() {
			this->draw();
		});
		
		
	}
	
	void add_node(Node* node) {
		mNodes.push_back(node);
	}
	
	void begin_link(Node& source) {
		
	}
	
	void end_link(Node& target) {
		
	}
	
	void draw() {
		render_pass().clear_color(0, background_color());
		mGrid->draw_content(nanogui::Matrix4f::identity(), mView, mProjection);
	}
	
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<Grid2d> mGrid;
	nanogui::Matrix4f mView;
	nanogui::Matrix4f mProjection;
	
	float mScrollX;
	float mScrollY;
	
	std::vector<blueprint::Node*> mNodes;

};

class NodeProcessor {
public:
	NodeProcessor(BlueprintCanvas& canvas)
	: mCanvas(canvas) {
		
	}
	
	int get_next_id() {
		return next_id++;
	}
	
	void build_node(Node& node){
		node.build();
	}
	
	void spawn_string_node(const nanogui::Vector2i& position) {
		auto node = std::make_unique<StringNode>(mCanvas, "String",  nanogui::Vector2i(196, 96), get_next_id(), get_next_id());
		node->set_position(position);
		build_node(*node);
		nodes.push_back(std::move(node));
		
		mCanvas.add_node(nodes.back().get());
	}

	void spawn_print_string_node(const nanogui::Vector2i& position) {
		auto node = std::make_unique<PrintStringNode>(mCanvas, "Print String",  nanogui::Vector2i(196, 96), get_next_id(), get_next_id(), get_next_id(), get_next_id());
		node->set_position(position);
		build_node(*node);
		nodes.push_back(std::move(node));
		mCanvas.add_node(nodes.back().get());
	}

	//
	//	Node& spawn_input_action_node(const std::string& key_string, int key_code) {
	//		auto label = "Input Action " + key_string;
	//		auto node = std::make_unique<Node>(get_next_id(), label.c_str(), nanogui::Color(255, 128, 128, 255));
	//		node->root_node = true;
	//		node->outputs.emplace_back(get_next_id(), node->id, "Pressed", PinType::Flow);
	//		node->outputs.emplace_back(get_next_id(), node->id, "Released", PinType::Flow);
	//
	//		node->evaluate = [node = node.get(), key_code]() {
	//			if (key_code != -1) {
	//				if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_PRESS) {
	//					node->outputs[0].can_flow = true;
	//				} else if (glfwGetKey(glfwGetCurrentContext(), key_code) == GLFW_RELEASE) {
	//					if (node->outputs[0].can_flow) {
	//						node->outputs[0].can_flow = false;
	//						node->outputs[1].can_flow = true;
	//					} else {
	//						node->outputs[1].can_flow = false;
	//					}
	//				}
	//			} else {
	//				node->outputs[0].can_flow = false;
	//				node->outputs[1].can_flow = false;
	//			}
	//		};
	//
	//		build_node(node.get());
	//		nodes.push_back(std::move(node));
	//		return *nodes.back().get();
	//	}
	
	void evaluate(Node& node) {
		if (node.evaluate) {
			node.evaluate();
		}
	}
	
	// Add more functions to define other nodes, and manage linking, deletion, etc.
	// ...
	
private:
	BlueprintCanvas& mCanvas;
	
	int next_id = 1;
	std::vector<std::unique_ptr<Node>> nodes;
	std::vector<std::unique_ptr<Link>> links;
	
	// Additional methods and logic needed for the complete functionality go here
	
};
}


class BlueprintPanel : public ScenePanel {
public:
	BlueprintPanel(Canvas& parent)
	: ScenePanel(parent, "Blueprint") {
		// Set the layout to horizontal with some padding
		set_layout(std::make_unique<nanogui::GroupLayout>(0, 0, 0));
		
		// Ensure the canvas is created with the correct dimensions or set it explicitly if needed
		mCanvas = std::make_unique<blueprint::BlueprintCanvas>(*this, parent.screen(), nanogui::Color(35, 65, 90, 255));
		
		mCanvas->set_fixed_size(nanogui::Vector2i(fixed_width(), parent.fixed_height() * 0.71));
				
		mNodeProcessor = std::make_unique<blueprint::NodeProcessor>(*mCanvas);
		
		mNodeProcessor->spawn_string_node(nanogui::Vector2i(mCanvas->fixed_size().x() / 4, mCanvas->fixed_size().y() / 4));

		mNodeProcessor->spawn_print_string_node(nanogui::Vector2i(mCanvas->fixed_size().x() / 2, mCanvas->fixed_size().y() / 2));
	}
	
private:
	void draw(NVGcontext *ctx) override {
		ScenePanel::draw(ctx);
	}
	
private:
	std::unique_ptr<blueprint::BlueprintCanvas> mCanvas;
	std::unique_ptr<blueprint::NodeProcessor> mNodeProcessor;
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
